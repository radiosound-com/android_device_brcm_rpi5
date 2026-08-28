// Copyright 2026 Radio Sound, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aidl/android/hardware/boot/BnBootControl.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace aidl::android::hardware::boot {

class BootControl final : public BnBootControl {
  public:
    BootControl();

    ::ndk::ScopedAStatus getActiveBootSlot(int32_t* result) override;
    ::ndk::ScopedAStatus getCurrentSlot(int32_t* result) override;
    ::ndk::ScopedAStatus getNumberSlots(int32_t* result) override;
    ::ndk::ScopedAStatus getSnapshotMergeStatus(MergeStatus* result) override;
    ::ndk::ScopedAStatus getSuffix(int32_t slot, std::string* result) override;
    ::ndk::ScopedAStatus isSlotBootable(int32_t slot, bool* result) override;
    ::ndk::ScopedAStatus isSlotMarkedSuccessful(int32_t slot, bool* result) override;
    ::ndk::ScopedAStatus markBootSuccessful() override;
    ::ndk::ScopedAStatus setActiveBootSlot(int32_t slot) override;
    ::ndk::ScopedAStatus setSlotAsUnbootable(int32_t slot) override;
    ::ndk::ScopedAStatus setSnapshotMergeStatus(MergeStatus status) override;

  private:
    struct Slot {
        uint8_t flags = 0;
        uint8_t reserved = 0;
    } __attribute__((packed));

    struct Control {
        char slot_suffix[4] = {};
        uint32_t magic = 0;
        uint8_t version = 0;
        uint8_t slot_count = 0;
        uint8_t reserved0 = 0;
        Slot slots[4] = {};
        uint8_t reserved1[9] = {};
        uint32_t crc32 = 0;
    } __attribute__((packed));

    static_assert(sizeof(Control) == 32);

    bool load();
    bool save();
    bool syncBootFiles(int current_slot, int trial_slot);
    bool writeAutoboot(int partition, int current_slot, int trial_slot);
    int currentSlot() const;
    bool validSlot(int slot) const;
    bool slotBootable(int slot) const;
    bool slotSuccessful(int slot) const;
    static uint32_t crc32(const uint8_t* data, size_t size);
    static uint8_t priority(const Slot& slot);
    static uint8_t tries(const Slot& slot);
    static bool successful(const Slot& slot);
    static void setState(Slot* slot, uint8_t priority, uint8_t tries, bool successful);

    Control control_;
};

}  // namespace aidl::android::hardware::boot
