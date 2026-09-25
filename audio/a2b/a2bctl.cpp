// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include "Profile.h"

using namespace aidl::android::hardware::audio::core::a2b;
using android::base::unique_fd;
namespace {
bool readProfile(const std::string& path, Profile* p, std::string* text) {
    if (path == "-") {
        char buffer[4096];
        ssize_t size;
        while ((size = TEMP_FAILURE_RETRY(read(STDIN_FILENO, buffer, sizeof(buffer)))) > 0) {
            if (text->size() + size > 262144) {
                std::cerr << "Profile exceeds 256 KiB\n";
                return false;
            }
            text->append(buffer, size);
        }
        std::string error;
        if (size < 0 || !parseProfile(*text, p, &error)) {
            std::cerr << "Invalid profile on stdin: " << error << '\n';
            return false;
        }
        return true;
    }
    unique_fd fd(open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    struct stat st{};
    std::string error;
    if (fd < 0 || fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size > 262144 ||
        !android::base::ReadFdToString(fd, text) || !parseProfile(*text, p, &error)) {
        std::cerr << "Invalid profile: " << path << " " << error << '\n';
        return false;
    }
    return true;
}
bool request(const std::string& command) {
    unique_fd fd(
        socket_local_client("a2b_control", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM));
    if (fd < 0) {
        perror("connect a2b_control (requires adb root)");
        return false;
    }
    timeval timeout{60, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    const std::string message = command + "\n";
    if (send(fd, message.data(), message.size(), MSG_NOSIGNAL) !=
        static_cast<ssize_t>(message.size()))
        return false;
    std::string response;
    char ch = 0;
    while (response.size() < 4096 && recv(fd, &ch, 1, 0) == 1 && ch != '\n') response += ch;
    std::cout << response << '\n';
    return response.starts_with("OK") && ch == '\n';
}
bool writeAtomic(const std::string& path, const std::string& text) {
    const auto temp = path + ".new";
    unique_fd fd(open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0640));
    if (fd < 0) {
        perror(temp.c_str());
        return false;
    }
    // audioserver AID 1041. Overrides remain writable only by root.
    const bool ok = fchown(fd, 0, 1041) == 0 && android::base::WriteStringToFd(text, fd) &&
                    fsync(fd) == 0 && rename(temp.c_str(), path.c_str()) == 0;
    if (!ok) unlink(temp.c_str());
    unique_fd dir(open("/data/vendor/a2b/profiles", O_DIRECTORY | O_RDONLY | O_CLOEXEC));
    return ok && dir >= 0 && fsync(dir) == 0;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr
            << "a2bctl validate FILE | install FILE | select ID | rollback ID | output usb|a2b\n"
               "a2bctl status | mute | unmute | shutdown | resume\n"
               "Use FILE=- to read a profile from stdin (e.g. shell redirection).\n";
        return 2;
    }
    const std::string command = argv[1];
    if (command != "validate" && getuid() != 0) {
        std::cerr << "Run with adb root\n";
        return 1;
    }
    if ((command == "validate" || command == "install") && argc == 3) {
        Profile p;
        std::string text;
        if (!readProfile(argv[2], &p, &text)) return 1;
        if (command == "validate") {
            std::cout << "Valid: " << p.id() << '\n';
            return 0;
        }
        unique_fd lock(open("/data/vendor/a2b/profiles/.lock",
                            O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600));
        if (lock < 0 || flock(lock, LOCK_EX)) return 1;
        const auto path = "/data/vendor/a2b/profiles/" + p.id() + ".json";
        std::string old;
        Profile previous;
        if (access(path.c_str(), F_OK) == 0) {
            if (!readProfile(path, &previous, &old) || !writeAtomic(path + ".previous", old))
                return 1;
        }
        if (!writeAtomic(path, text)) {
            perror("install");
            return 1;
        }
        std::cout << "Installed " << p.id() << "; apply with a2bctl select " << p.id() << '\n';
        return 0;
    }
    if (command == "rollback" && argc == 3 && validId(argv[2])) {
        const std::string path = std::string("/data/vendor/a2b/profiles/") + argv[2] + ".json";
        unique_fd lock(open("/data/vendor/a2b/profiles/.lock",
                            O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600));
        if (lock < 0 || flock(lock, LOCK_EX)) return 1;
        Profile previous;
        std::string text;
        if (!readProfile(path + ".previous", &previous, &text) || previous.id() != argv[2] ||
            !writeAtomic(path, text))
            return 1;
        return request(std::string("reload ") + argv[2]) ? 0 : 1;
    }
    if (command == "select" && argc == 3 && validId(argv[2]))
        return request(std::string("reload ") + argv[2]) ? 0 : 1;
    if (command == "output" && argc == 3) {
        const std::string output = argv[2];
        if (output != "usb" && output != "a2b") return 2;
        if (output == "a2b") {
            bool found = false;
            for (int i = 0; i < 8; ++i) {
                std::string id;
                if (android::base::ReadFileToString("/proc/asound/card" + std::to_string(i) + "/id",
                                                    &id) &&
                    android::base::Trim(id) == "ad242x")
                    found = true;
            }
            if (!found) {
                std::cerr << "A2B overlay absent; enable it and reboot first\n";
                return 1;
            }
        }
        if (!request("quiesce")) return 1;
        if (!android::base::SetProperty("persist.vendor.audio.device",
                                        output == "a2b" ? "rpi" : "usb") ||
            !android::base::SetProperty("vendor.audio.a2b.restart", "1")) {
            std::cerr << "Route/restart failed; check properties and use a2bctl resume\n";
            return 1;
        }
        std::cout << "Audio restart requested; playback will be interrupted\n";
        return 0;
    }
    if (argc == 2 && (command == "status" || command == "mute" || command == "unmute" ||
                      command == "shutdown" || command == "resume"))
        return request(command) ? 0 : 1;
    std::cerr << "Invalid command or arguments\n";
    return 2;
}
