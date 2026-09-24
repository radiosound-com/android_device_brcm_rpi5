// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <utils/Errors.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "Profile.h"

namespace aidl::android::hardware::audio::core::a2b {
class A2bController final {
   public:
    static A2bController& getInstance();
    void startControlServer();
    // Playback clients attach after the persistent PCM establishes clocks.
    // Releasing the last client never shuts down the bus or stops clocks.
    void acquire();
    void release();
    bool allowAudio() const { return mAllowAudio.load() && !mClockFailed.load(); }
    void clockFailed() {
        mClockFailed = true;
        mAllowAudio = false;
    }

   private:
    A2bController() = default;
    bool load(const std::string& id, Profile* profile);
    bool initialize();
    bool stop();
    bool run(const std::string& phase);
    std::string command(const std::string& request);
    void checkHealth();
    std::mutex mLock;
    std::once_flag mServerOnce;
    std::unique_ptr<Transport> mTransport;
    Profile mProfile;
    std::string mError;
    unsigned mUsers = 0;
    bool mReady = false;
    bool mClockStarted = false;
    bool mQuiesced = false;
    std::optional<bool> mMuted;
    std::atomic<bool> mAllowAudio = false;
    std::atomic<bool> mClockFailed = false;
};
}  // namespace aidl::android::hardware::audio::core::a2b
