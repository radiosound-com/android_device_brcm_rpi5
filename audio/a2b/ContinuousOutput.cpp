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
    {
        std::lock_guard lock(mLock);
        mRunning = false;
    }
    mSpaceAvailable.notify_all();
    mThread.join();
}
ContinuousOutput::Client ContinuousOutput::attach() {
    std::lock_guard lock(mLock);
    const Client id = mNextClient++;
    mClients.try_emplace(id);
    return id;
}
void ContinuousOutput::detach(Client client) {
    {
        std::lock_guard lock(mLock);
        if (auto it = mClients.find(client); it != mClients.end()) {
            mStatistics.canceledFrames += it->second.count / 2;
            mClients.erase(it);
        }
    }
    mSpaceAvailable.notify_all();
}
void ContinuousOutput::clear(Client client) {
    {
        std::lock_guard lock(mLock);
        if (auto it = mClients.find(client); it != mClients.end()) {
            auto& q = it->second;
            mStatistics.canceledFrames += q.count / 2;
            q.count = 0;
            ++q.generation;
            q.started = q.underrunning = false;
        }
    }
    mSpaceAvailable.notify_all();
}
size_t ContinuousOutput::enqueue(Client client, std::span<const int16_t> samples) {
    std::unique_lock lock(mLock);
    auto it = mClients.find(client);
    if (it == mClients.end()) return 0;
    const uint64_t generation = it->second.generation;
    const size_t total = samples.size() / 2 * 2;
    size_t copied = 0;
    while (copied < total) {
        it = mClients.find(client);
        if (!mRunning || it == mClients.end() || it->second.generation != generation) {
            mStatistics.canceledFrames += (total - copied) / 2;
            break;
        }
        auto& q = it->second;
        if (q.count == q.samples.size()) {
            ++mStatistics.producerWaits;
            mSpaceAvailable.wait(lock, [&] {
                const auto current = mClients.find(client);
                return !mRunning || current == mClients.end() ||
                        current->second.generation != generation ||
                        current->second.count < current->second.samples.size();
            });
            continue; // Re-find the queue after a possible flush/detach.
        }
        const size_t n = std::min(total - copied, q.samples.size() - q.count);
        for (size_t i = 0; i < n; ++i)
            q.samples[(q.read + q.count + i) % q.samples.size()] =
                    int32_t(samples[copied + i]) * 65536;
        q.count += n;
        q.started = true;
        copied += n;
    }
    return copied / 2;
}
size_t ContinuousOutput::queuedFrames(Client client) const {
    std::lock_guard lock(mLock);
    auto it = mClients.find(client);
    return it != mClients.end() ? it->second.count / 2 : 0;
}
ContinuousOutput::Statistics ContinuousOutput::statistics() const {
    std::lock_guard lock(mLock);
    auto result = mStatistics;
    for (const auto& [id, q] : mClients) result.queuedFrames += q.count / 2;
    return result;
}
void ContinuousOutput::run() {
    if (mThreadInit) mThreadInit();
    std::array<int32_t, kPeriodFrames * 2> samples{};
    std::array<int64_t, kPeriodFrames * 2> mix{};
    while (mRunning) {
        mix.fill(0);
        const bool audible = mAudible();
        {
            std::lock_guard lock(mLock);
            for (auto& [id, q] : mClients) {
                const size_t n = std::min(q.count, mix.size());
                for (size_t i = 0; i < n; ++i) mix[i] += q.samples[(q.read + i) % q.samples.size()];
                q.read = (q.read + n) % q.samples.size();
                q.count -= n;
                const bool starved = audible && q.started && n < mix.size();
                if (starved) {
                    if (!q.underrunning) ++mStatistics.underruns;
                    mStatistics.starvedFrames += (mix.size() - n) / 2;
                }
                q.underrunning = starved;
            }
        }
        mSpaceAvailable.notify_all();
        // Consume/discard queued audio while muted so it cannot play on unmute.
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
