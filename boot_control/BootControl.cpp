// Copyright 2026 Radio Sound, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "BootControl.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace aidl::android::hardware::boot {

namespace {

constexpr char kMiscDevice[] = "/dev/block/by-name/misc";
constexpr off_t kControlOffset = 2048;
constexpr uint32_t kMagic = 0x42414342;
constexpr uint8_t kVersion = 1;
constexpr int kSlotCount = 2;
constexpr unsigned kDefaultTries = 7;

const char* SlotSuffix(int slot) {
    return slot == 0 ? "_a" : "_b";
}

const char* SlotPartition(int slot) {
    return slot == 0 ? "1" : "2";
}

bool MakeBlockDeviceWritable(const char* block) {
    ::android::base::unique_fd fd(open(block, O_RDONLY | O_CLOEXEC));
    if (fd == -1) {
        PLOG(ERROR) << "open " << block << " for BLKROSET";
        return false;
    }

    int read_only = -1;
    if (ioctl(fd.get(), BLKROGET, &read_only) == 0) {
        LOG(INFO) << "block device " << block << " read_only_before=" << read_only;
        if (read_only == 0) return true;
    } else {
        PLOG(ERROR) << "BLKROGET " << block;
    }

    int writable = 0;
    if (ioctl(fd.get(), BLKROSET, &writable) != 0) {
        PLOG(ERROR) << "BLKROSET " << block << " read_only=0";
        return false;
    }

    read_only = -1;
    if (ioctl(fd.get(), BLKROGET, &read_only) != 0 || read_only != 0) {
        PLOG(ERROR) << "BLKROGET verify " << block << " read_only=" << read_only;
        return false;
    }
    LOG(INFO) << "block device " << block << " read_only_after=0";
    return true;
}

}  // namespace

BootControl::BootControl() {
    if (!load()) {
        const int slot = currentSlot();
        memset(&control_, 0, sizeof(control_));
        memcpy(control_.slot_suffix, SlotSuffix(slot), 3);
        control_.magic = kMagic;
        control_.version = kVersion;
        control_.slot_count = kSlotCount;
        for (int i = 0; i < kSlotCount; ++i) {
            setState(&control_.slots[i], 7, kDefaultTries, i == slot);
        }
        save();
    }
}

uint32_t BootControl::crc32(const uint8_t* data, size_t size) {
    uint32_t value = ~0U;
    for (size_t i = 0; i < size; ++i) {
        value ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            value = (value >> 1) ^ (0xedb88320U & -(value & 1));
        }
    }
    return ~value;
}

uint8_t BootControl::priority(const Slot& slot) { return slot.flags & 0x0f; }
uint8_t BootControl::tries(const Slot& slot) { return (slot.flags >> 4) & 0x07; }
bool BootControl::successful(const Slot& slot) { return (slot.flags & 0x80) != 0; }

void BootControl::setState(Slot* slot, uint8_t slot_priority, uint8_t slot_tries,
                           bool slot_successful) {
    slot->flags = (slot_priority & 0x0f) | ((slot_tries & 0x07) << 4) |
                  (slot_successful ? 0x80 : 0);
}

bool BootControl::load() {
    ::android::base::unique_fd fd(open(kMiscDevice, O_RDONLY | O_CLOEXEC));
    if (fd == -1) return false;
    Control candidate;
    if (pread(fd, &candidate, sizeof(candidate), kControlOffset) != sizeof(candidate)) {
        return false;
    }
    const uint32_t expected = crc32(reinterpret_cast<const uint8_t*>(&candidate),
                                     offsetof(Control, crc32));
    if (candidate.magic != kMagic || candidate.version != kVersion ||
        candidate.slot_count != kSlotCount || candidate.crc32 != expected ||
        (strcmp(candidate.slot_suffix, "_a") != 0 && strcmp(candidate.slot_suffix, "_b") != 0)) {
        return false;
    }
    control_ = candidate;
    return true;
}

bool BootControl::save() {
    control_.crc32 = crc32(reinterpret_cast<const uint8_t*>(&control_),
                            offsetof(Control, crc32));
    ::android::base::unique_fd fd(open(kMiscDevice, O_RDWR | O_SYNC | O_CLOEXEC));
    if (fd == -1) {
        PLOG(ERROR) << "open " << kMiscDevice;
        return false;
    }
    return pwrite(fd, &control_, sizeof(control_), kControlOffset) == sizeof(control_);
}

int BootControl::currentSlot() const {
    const std::string suffix = ::android::base::GetProperty("ro.boot.slot_suffix", "_a");
    return suffix == "_b" ? 1 : 0;
}

bool BootControl::validSlot(int slot) const { return slot >= 0 && slot < kSlotCount; }
bool BootControl::slotBootable(int slot) const {
    return validSlot(slot) && priority(control_.slots[slot]) != 0 &&
           (tries(control_.slots[slot]) != 0 || successful(control_.slots[slot]));
}
bool BootControl::slotSuccessful(int slot) const {
    return validSlot(slot) && successful(control_.slots[slot]);
}

bool BootControl::writeAutoboot(int partition, int current_slot, int trial_slot) {
    const char* mount_path = partition == 1 ? "/mnt/caramel-vanilla-boot-a"
                                            : "/mnt/caramel-vanilla-boot-b";
    const char* block = partition == 1 ? "/dev/block/by-name/boot_a"
                                       : "/dev/block/by-name/boot_b";
    if (!MakeBlockDeviceWritable(block)) return false;
    errno = 0;
    const int mkdir_result = mkdir(mount_path, 0755);
    const int mkdir_errno = errno;
    if (mkdir_result != 0 && mkdir_errno != EEXIST) {
        errno = mkdir_errno;
        PLOG(ERROR) << "mkdir " << mount_path;
        return false;
    }
    LOG(INFO) << "writeAutoboot partition=" << partition << " block=" << block
              << " mount_path=" << mount_path << " mkdir_result=" << mkdir_result
              << " mkdir_errno=" << mkdir_errno;

    errno = 0;
    const int mount_result = mount(block, mount_path, "vfat", 0, nullptr);
    const int mount_errno = errno;
    const bool already_mounted = mount_result != 0 && mount_errno == EBUSY;
    LOG(INFO) << "writeAutoboot partition=" << partition << " mount_result="
              << mount_result << " mount_errno=" << mount_errno
              << " already_mounted=" << already_mounted;
    if (mount_result != 0 && !already_mounted) {
        errno = mount_errno;
        PLOG(ERROR) << "mount " << block;
        return false;
    }
    const std::string path = std::string(mount_path) + "/autoboot.txt";
    const std::string content = std::string("[all]\ntryboot_a_b=1\nboot_partition=") +
                                SlotPartition(current_slot) + "\n[tryboot]\nboot_partition=" +
                                SlotPartition(trial_slot) + "\n";
    const bool ok = ::android::base::WriteStringToFile(content, path, 0, 0, false);
    if (!ok) PLOG(ERROR) << "write " << path;
    sync();
    if (!already_mounted) {
        errno = 0;
        const int umount_result = umount(mount_path);
        const int umount_errno = errno;
        LOG(INFO) << "writeAutoboot partition=" << partition << " umount_result="
                  << umount_result << " umount_errno=" << umount_errno;
        if (umount_result != 0) {
            errno = umount_errno;
            PLOG(ERROR) << "umount " << mount_path;
        }
    }
    return ok;
}

bool BootControl::syncBootFiles(int current_slot, int trial_slot) {
    return writeAutoboot(1, current_slot, trial_slot) &&
           writeAutoboot(2, current_slot, trial_slot);
}

::ndk::ScopedAStatus BootControl::getActiveBootSlot(int32_t* result) {
    *result = strcmp(control_.slot_suffix, "_b") == 0 ? 1 : 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::getCurrentSlot(int32_t* result) {
    *result = currentSlot();
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::getNumberSlots(int32_t* result) {
    *result = kSlotCount;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::getSnapshotMergeStatus(MergeStatus* result) {
    *result = MergeStatus::NONE;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::getSuffix(int32_t slot, std::string* result) {
    if (!validSlot(slot)) {
        result->clear();
    } else {
        *result = SlotSuffix(slot);
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::isSlotBootable(int32_t slot, bool* result) {
    if (!validSlot(slot)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::INVALID_SLOT, "invalid slot");
    }
    *result = slotBootable(slot);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::isSlotMarkedSuccessful(int32_t slot, bool* result) {
    if (!validSlot(slot)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::INVALID_SLOT, "invalid slot");
    }
    *result = slotSuccessful(slot);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::markBootSuccessful() {
    const int current = currentSlot();
    setState(&control_.slots[current], 15, kDefaultTries, true);
    memcpy(control_.slot_suffix, SlotSuffix(current), 3);
    if (!save() || !syncBootFiles(current, 1 - current)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::COMMAND_FAILED, "could not commit boot slot");
    }
    ::android::base::SetProperty("vendor.rpi5.ab.trial", "0");
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::setActiveBootSlot(int32_t slot) {
    if (!validSlot(slot)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::INVALID_SLOT, "invalid slot");
    }
    const int current = currentSlot();
    setState(&control_.slots[slot], 15, kDefaultTries, false);
    memcpy(control_.slot_suffix, SlotSuffix(slot), 3);
    if (!save() || !syncBootFiles(current, slot)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::COMMAND_FAILED, "could not schedule boot slot");
    }
    if (slot != current) ::android::base::SetProperty("vendor.rpi5.ab.trial", "1");
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BootControl::setSlotAsUnbootable(int32_t slot) {
    if (!validSlot(slot)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::INVALID_SLOT, "invalid slot");
    }
    setState(&control_.slots[slot], 0, 0, false);
    return save() ? ::ndk::ScopedAStatus::ok()
                  : ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                            IBootControl::COMMAND_FAILED, "could not save slot state");
}

::ndk::ScopedAStatus BootControl::setSnapshotMergeStatus(MergeStatus status) {
    if (status != MergeStatus::NONE) {
        return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IBootControl::COMMAND_FAILED, "snapshot merge is not enabled");
    }
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::boot
