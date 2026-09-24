// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

namespace aidl::android::hardware::audio::core::a2b {
// The PCM writer belongs to the service, never to a playback client. Removing
// the last client only changes the samples to zero; it cannot close the device.
class ContinuousOutput final {
  public:
    static constexpr size_t kPeriodFrames = 240;
    static constexpr size_t kQueueFrames = 4096;
    using Client = uint64_t;
    using Write = std::function<bool(std::span<const int32_t>)>;
    ContinuousOutput(Write write, std::function<bool()> audible,
                     std::function<void()> fault, std::function<void()> threadInit = {});
    ~ContinuousOutput();  // For scoped tests. Production instance has process lifetime.
    Client attach();
    void detach(Client client);
    void clear(Client client);
    size_t enqueue(Client client, std::span<const int16_t> samples);
    size_t queuedFrames(Client client) const;
    uint64_t framesWritten() const { return mFramesWritten.load(); }
    uint64_t writeErrors() const { return mWriteErrors.load(); }
    bool healthy() const { return mHealthy.load(); }

  private:
    struct Queue {
        std::vector<int32_t> samples = std::vector<int32_t>(kQueueFrames * 2);
        size_t read = 0, count = 0;
    };
    void run();
    const Write mWrite;
    const std::function<bool()> mAudible;
    const std::function<void()> mFault, mThreadInit;
    mutable std::mutex mLock;
    std::map<Client, Queue> mClients;
    Client mNextClient = 1;
    std::atomic<bool> mRunning = true, mHealthy = false;
    std::atomic<uint64_t> mFramesWritten = 0, mWriteErrors = 0;
    std::thread mThread;
};
}  // namespace aidl::android::hardware::audio::core::a2b
