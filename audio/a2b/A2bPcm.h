// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#pragma once
#include "ContinuousOutput.h"
#include <memory>
#include <string>
#include <tinyalsa/asoundlib.h>
#include <utils/Errors.h>

namespace aidl::android::hardware::audio::core::a2b {
class A2bPcm final {
  public:
    static A2bPcm& getInstance();
    ::android::status_t start(int card, int device);
    ContinuousOutput& output() { return *mOutput; }
    std::string status();
    static constexpr unsigned kPeriodCount = 8;
    static constexpr unsigned kLatencyMs = ContinuousOutput::kPeriodFrames * kPeriodCount / 48;

  private:
    A2bPcm() = default;
    std::mutex mLock;
    pcm* mPcm = nullptr;
    int mCard = -1, mDevice = -1;
    std::unique_ptr<ContinuousOutput> mOutput;
};
}  // namespace aidl::android::hardware::audio::core::a2b
