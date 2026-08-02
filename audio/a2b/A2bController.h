/*
 * Copyright 2026 Radio Sound, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>

#include <utils/Errors.h>

namespace aidl::android::hardware::audio::core::a2b {

// Process-wide A2B lifecycle. The interface is deliberately small; transport
// and command execution remain implementation details of the HAL module.
class A2bController final {
  public:
    static A2bController& getInstance();

    // Must be called after ALSA has opened the PCM path so the A2B clock is live.
    // A failed attempt is retryable on the next stream start.
    ::android::status_t initialize();

  private:
    A2bController() = default;

    std::mutex mLock;
    bool mInitialized = false;
};

}  // namespace aidl::android::hardware::audio::core::a2b
