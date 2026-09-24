// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#include "ContinuousOutput.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>

namespace aidl::android::hardware::audio::core::a2b {
ContinuousOutput::ContinuousOutput(Write write, std::function<bool()> audible,
                                   std::function<void()> fault, std::function<void()> threadInit)
    : mWrite(std::move(write)), mAudible(std::move(audible)), mFault(std::move(fault)),
      mThreadInit(std::move(threadInit)), mThread(&ContinuousOutput::run, this) {}
ContinuousOutput::~ContinuousOutput() {
    mRunning = false;
    mThread.join();
}
ContinuousOutput::Client ContinuousOutput::attach() {
    std::lock_guard lock(mLock);
    const Client id = mNextClient++;
    mClients.try_emplace(id);
    return id;
}
void ContinuousOutput::detach(Client client) {
    std::lock_guard lock(mLock);
    mClients.erase(client);
}
void ContinuousOutput::clear(Client client) {
    std::lock_guard lock(mLock);
    if (auto it = mClients.find(client); it != mClients.end()) it->second.count = 0;
}
size_t ContinuousOutput::enqueue(Client client, std::span<const int16_t> samples) {
    std::lock_guard lock(mLock);
    auto it = mClients.find(client);
    if (it == mClients.end()) return 0;
    auto& q = it->second;
    const size_t n = std::min(samples.size() / 2 * 2, q.samples.size() - q.count);
    for (size_t i = 0; i < n; ++i)
        q.samples[(q.read + q.count + i) % q.samples.size()] = int32_t(samples[i]) * 65536;
    q.count += n;
    return n / 2;
}
size_t ContinuousOutput::queuedFrames(Client client) const {
    std::lock_guard lock(mLock);
    auto it = mClients.find(client);
    return it != mClients.end() ? it->second.count / 2 : 0;
}
void ContinuousOutput::run() {
    if (mThreadInit) mThreadInit();
    std::array<int32_t, kPeriodFrames * 2> samples{};
    std::array<int64_t, kPeriodFrames * 2> mix{};
    while (mRunning) {
        mix.fill(0);
        {
            std::lock_guard lock(mLock);
            for (auto& [id, q] : mClients) {
                const size_t n = std::min(q.count, mix.size());
                for (size_t i = 0; i < n; ++i) mix[i] += q.samples[(q.read + i) % q.samples.size()];
                q.read = (q.read + n) % q.samples.size();
                q.count -= n;
            }
        }
        // Consume/discard queued audio while muted so it cannot play on unmute.
        const bool audible = mAudible();
        for (size_t i = 0; i < samples.size(); ++i)
            samples[i] = audible ? std::clamp<int64_t>(mix[i], INT32_MIN, INT32_MAX) : 0;
        if (mWrite(samples)) {
            mFramesWritten += kPeriodFrames;
            mHealthy = true;
        } else {
            mHealthy = false;
            ++mWriteErrors;
            mFault();
            // Keep trying the same PCM, including after an amplifier/link fault.
            // A hardware/driver error is reported, never treated as idle shutdown.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
}  // namespace aidl::android::hardware::audio::core::a2b
