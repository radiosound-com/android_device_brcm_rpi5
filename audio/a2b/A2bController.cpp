// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#define LOG_TAG "AHAL_A2B"
#include "A2bController.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <thread>

namespace aidl::android::hardware::audio::core::a2b {
namespace {
using ::android::base::unique_fd;
constexpr char kProfileProperty[] = "persist.vendor.audio.a2b.profile";
class I2cTransport final : public Transport {
   public:
    explicit I2cTransport(int bus)
        : mFd(open(("/dev/i2c-" + std::to_string(bus)).c_str(), O_RDWR | O_CLOEXEC)) {}
    bool write(int address, int reg, int value) override {
        i2c_smbus_data data = {};
        data.byte = value;
        return transfer(address, reg, I2C_SMBUS_WRITE, &data);
    }
    bool read(int address, int reg, int* value) override {
        i2c_smbus_data data = {};
        if (!transfer(address, reg, I2C_SMBUS_READ, &data)) return false;
        *value = data.byte;
        return true;
    }
    void delay(int ms) override { usleep(ms * 1000); }

   private:
    bool transfer(int address, int reg, int direction, i2c_smbus_data* data) {
        if (mFd < 0 || ioctl(mFd, I2C_SLAVE, address) < 0) return false;
        i2c_smbus_ioctl_data args = {};
        args.read_write = direction;
        args.command = reg;
        args.size = I2C_SMBUS_BYTE_DATA;
        args.data = data;
        return ioctl(mFd, I2C_SMBUS, &args) == 0;
    }
    unique_fd mFd;
};
}  // namespace

A2bController& A2bController::getInstance() {
    // Process lifetime: the server must not race static destruction at exit.
    static auto* instance = new A2bController;
    return *instance;
}
bool A2bController::load(const std::string& id, Profile* profile) {
    if (!validId(id)) {
        mError = "invalid profile ID";
        return false;
    }
    std::string path = "/data/vendor/a2b/profiles/" + id + ".json";
    struct stat st{};
    if (lstat(path.c_str(), &st) < 0) {
        if (errno != ENOENT) {
            mError = "cannot inspect override";
            return false;
        }
        path = "/vendor/etc/a2b/profiles/" + id + ".json";
    }
    unique_fd fd(open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (fd < 0 || fstat(fd.get(), &st) < 0 || !S_ISREG(st.st_mode) || st.st_uid != 0 ||
        (st.st_mode & 0022) || st.st_size > 262144) {
        mError =
            "profile must be root-owned, regular, <=256 KiB, not group/world writable: " + path;
        return false;
    }
    std::string text;
    if (!::android::base::ReadFdToString(fd, &text) || !parseProfile(text, profile, &mError))
        return false;
    if (profile->id() != id) {
        mError = "filename/profile ID mismatch";
        return false;
    }
    return true;
}
bool A2bController::run(const std::string& phase) {
    if (!mTransport) return true;
    const bool ok = execute(mProfile, phase, *mTransport, &mError);
    if (!ok) LOG(ERROR) << "A2B " << mProfile.id() << ": " << mError;
    return ok;
}
bool A2bController::stop() {
    mAllowAudio = false;
    mReady = false;
    // Attempt shutdown even if mute failed (e.g. after a bus interruption).
    const bool muted = run("mute");
    const bool stopped = run("shutdown");
    return muted && stopped;
}
bool A2bController::initialize() {
    mAllowAudio = false;
    mReady = false;
    mTransport = std::make_unique<I2cTransport>(mProfile.data["i2c_bus"].asInt());
    if (!run("init") || !verifyNodes(mProfile, *mTransport, &mError) || !run("health") ||
        !run("mute")) {
        const std::string error = mError;
        stop();
        mError = error;
        return false;
    }
    mReady = true;
    if (mClockFailed) {
        stop();
        mError = "PCM clock/write failure";
        return false;
    }
    if (!mMuted.value_or(mProfile.data["start_muted"].asBool())) {
        if (!run("unmute")) {
            stop();
            return false;
        }
        mAllowAudio = true;
    }
    mError.clear();
    return true;
}
::android::status_t A2bController::acquire() {
    std::lock_guard lock(mLock);
    if (mQuiesced) return ::android::INVALID_OPERATION;
    if (mUsers != 0) {
        if (!mReady) return ::android::NO_INIT;
        ++mUsers;
        return ::android::OK;
    }
    mClockFailed = false;
    const std::string id = ::android::base::GetProperty(kProfileProperty, "tas5720a_1node");
    // File replacement is staging only. Use reload to adopt edits; standby must
    // not silently retry a rejected override after a successful in-memory rollback.
    if ((mProfile.id() != id && !load(id, &mProfile)) || !initialize()) {
        LOG(ERROR) << "A2B start failed: " << mError;
        mTransport.reset();
        return ::android::NO_INIT;
    }
    mUsers = 1;
    return ::android::OK;
}
void A2bController::release() {
    std::lock_guard lock(mLock);
    if (mUsers && --mUsers == 0) {
        stop();
        mTransport.reset();
    }
}
std::string A2bController::command(const std::string& request) {
    std::lock_guard lock(mLock);
    bool ok = true;
    if (request == "status") {
        return "OK profile=" + mProfile.id() + " streams=" + std::to_string(mUsers) +
               " ready=" + std::to_string(mReady) +
               " audible=" + std::to_string(mAllowAudio.load()) +
               " quiesced=" + std::to_string(mQuiesced) + " error=" + mError;
    } else if (request.starts_with("reload ")) {
        Profile candidate;
        if (!load(request.substr(7), &candidate)) return "ERROR " + mError;
        const Profile previous = mProfile;
        if (mUsers && !stop()) return "ERROR " + mError;
        mTransport.reset();
        mProfile = std::move(candidate);
        mMuted = true;
        if (mUsers && !initialize()) {
            const std::string error = mError;
            mProfile = previous;
            initialize();  // Attempt rollback, still muted. Report original failure.
            return "ERROR " + error +
                   "; previous profile restored, ready=" + std::to_string(mReady);
        }
        ok = ::android::base::SetProperty(kProfileProperty, mProfile.id());
        if (!ok) mError = "profile applied but could not persist selection";
    } else if (request == "mute") {
        mAllowAudio = false;
        mMuted = true;
        if (mUsers) ok = run("mute");
    } else if (request == "unmute") {
        if (mQuiesced || (mUsers && !mReady)) return "ERROR resume/reload before unmute";
        mMuted = false;
        if (mUsers) {
            ok = !mClockFailed && run("health") && run("unmute");
            if (!ok) {
                stop();
                mMuted = true;
            }
            mAllowAudio = ok;
        }
    } else if (request == "shutdown" || request == "quiesce") {
        mMuted = true;
        mQuiesced = true;
        if (mUsers) ok = stop();
    } else if (request == "resume") {
        mQuiesced = false;
        mMuted = true;
        mClockFailed = false;
        if (mUsers) {
            stop();
            ok = initialize();
        }
    } else
        return "ERROR unknown command";
    return ok ? "OK" : "ERROR " + mError;
}
void A2bController::checkHealth() {
    std::lock_guard lock(mLock);
    if (!mUsers || !mReady) return;
    if (mClockFailed || !run("health")) {
        const std::string error = mClockFailed ? "PCM clock/write failure" : mError;
        mMuted = true;
        stop();
        mError = error + "; resume and unmute after correcting the fault";
        LOG(ERROR) << mError;
    }
}
void A2bController::startControlServer() {
    std::call_once(mServerOnce, [this] {
        const int server = android_get_control_socket("a2b_control");
        if (server < 0 || listen(server, 4) < 0) {
            LOG(ERROR) << "A2B control socket unavailable";
            return;
        }
        std::thread([this, server] {
            for (;;) {
                pollfd pfd{server, POLLIN, 0};
                if (poll(&pfd, 1, 1000) > 0 && (pfd.revents & POLLIN)) {
                    unique_fd client(accept4(server, nullptr, nullptr, SOCK_CLOEXEC));
                    ucred peer{};
                    socklen_t size = sizeof(peer);
                    if (client < 0 || getsockopt(client, SOL_SOCKET, SO_PEERCRED, &peer, &size) ||
                        peer.uid != 0)
                        continue;
                    timeval timeout{2, 0};
                    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                    std::string request;
                    char ch = 0;
                    while (request.size() < 128 && recv(client, &ch, 1, 0) == 1 && ch != '\n')
                        request += ch;
                    const std::string reply =
                        (ch == '\n' ? command(request) : "ERROR invalid request") + "\n";
                    send(client, reply.data(), reply.size(), MSG_NOSIGNAL);
                }
                checkHealth();
            }
        }).detach();
    });
}
}  // namespace aidl::android::hardware::audio::core::a2b
