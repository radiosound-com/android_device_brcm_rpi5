// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#define LOG_TAG "AHAL_A2bPcm"
#include "A2bPcm.h"
#include "A2bController.h"
#include <android-base/logging.h>
#include <hardware_legacy/power.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

namespace aidl::android::hardware::audio::core::a2b {
A2bPcm& A2bPcm::getInstance() {
    // A2B clocks belong to the powered system, not Android playback streams.
    // The OS closes this PCM only when the HAL process exits/restarts.
    static auto* instance = new A2bPcm;
    return *instance;
}
::android::status_t A2bPcm::start(int card, int device) {
    std::lock_guard lock(mLock);
    if (mOutput) {
        if (card != mCard || device != mDevice) return ::android::BAD_VALUE;
        return mOutput->healthy() && mOutput->framesWritten() >=
                ContinuousOutput::kPeriodFrames * (kPeriodCount + 1)
                ? ::android::OK : ::android::NO_INIT;
    }
    if (acquire_wake_lock(PARTIAL_WAKE_LOCK, "rpi-a2b-continuous-clock") != 0) {
        LOG(ERROR) << "Cannot prevent suspend for the continuous A2B clock";
        return ::android::NO_INIT;
    }
    pcm_config config{};
    config.channels = 2;
    config.rate = 48000;
    config.format = PCM_FORMAT_S32_LE;
    config.period_size = ContinuousOutput::kPeriodFrames;
    config.period_count = kPeriodCount;
    config.start_threshold = config.period_size * config.period_count;
    mPcm = pcm_open(card, device, PCM_OUT | PCM_MONOTONIC, &config);
    if (!mPcm || !pcm_is_ready(mPcm)) {
        LOG(ERROR) << "Cannot open A2B PCM: " << (mPcm ? pcm_get_error(mPcm) : "null PCM");
        if (mPcm) pcm_close(mPcm);
        mPcm = nullptr;
        release_wake_lock("rpi-a2b-continuous-clock");
        return ::android::NO_INIT;
    }
    mCard = card;
    mDevice = device;
    mOutput = std::make_unique<ContinuousOutput>(
            [this](std::span<const int32_t> samples) {
                size_t offset = 0;
                while (offset < samples.size() / 2) {
                    const int n = pcm_writei(mPcm, samples.data() + offset * 2,
                                             samples.size() / 2 - offset);
                    if (n <= 0) return false;
                    offset += n;
                }
                return true;
            },
            [] { return A2bController::getInstance().allowAudio(); },
            [] { A2bController::getInstance().clockFailed(); },
            [] {
                pthread_setname_np(pthread_self(), "a2b_clock");
                sched_param priority{.sched_priority = 2};
                if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &priority) != 0)
                    LOG(WARNING) << "Could not set A2B clock thread realtime priority";
            });
    for (int i = 0; i < 1000; ++i) {
        if (mOutput->framesWritten() >= config.start_threshold + config.period_size) {
            LOG(INFO) << "Continuous A2B PCM started: 48 kHz stereo, 32-bit, card " << card;
            return ::android::OK;
        }
        usleep(1000);
    }
    // Retain the writer and suspend blocker even when startup times out.
    LOG(ERROR) << "A2B clock startup timed out; writer retained for recovery";
    return ::android::NO_INIT;
}
std::string A2bPcm::status() {
    std::lock_guard lock(mLock);
    return std::string(" clock=") + (mOutput ? (mOutput->healthy() ? "running" : "error") : "not_started") +
           " clock_frames=" + std::to_string(mOutput ? mOutput->framesWritten() : 0) +
           " clock_errors=" + std::to_string(mOutput ? mOutput->writeErrors() : 0);
}
}  // namespace aidl::android::hardware::audio::core::a2b
