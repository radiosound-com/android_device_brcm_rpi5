// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#include "ContinuousOutput.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <future>

namespace aidl::android::hardware::audio::core::a2b {
namespace {
class Sink {
  public:
    bool write(std::span<const int32_t> samples) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::lock_guard lock(mutex);
        blocks.emplace_back(samples.begin(), samples.end());
        changed.notify_all();
        return !fail.load();
    }
    size_t count() { std::lock_guard lock(mutex); return blocks.size(); }
    bool wait(size_t n) {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, std::chrono::seconds(2), [&] { return blocks.size() >= n; });
    }
    bool zeroSince(size_t n) {
        std::lock_guard lock(mutex);
        return std::all_of(blocks.begin() + n, blocks.end(), [](const auto& b) {
            return std::all_of(b.begin(), b.end(), [](int32_t v) { return v == 0; });
        });
    }
    bool contains(int32_t value) {
        std::lock_guard lock(mutex);
        return std::any_of(blocks.begin(), blocks.end(), [&](const auto& b) {
            return std::find(b.begin(), b.end(), value) != b.end();
        });
    }
    std::atomic<bool> fail = false;
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<std::vector<int32_t>> blocks;
};
// Freeze the PCM writer at period boundaries, independent of host scheduling.
class GatedSink {
  public:
    bool write(std::span<const int32_t> samples) {
        std::unique_lock lock(mMutex);
        mBlocks.emplace_back(samples.begin(), samples.end());
        mChanged.notify_all();
        mChanged.wait(lock, [&] { return mFree || mBlocks.size() <= mReleased; });
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
    }
    bool wait(size_t blocks) {
        std::unique_lock lock(mMutex);
        return mChanged.wait_for(lock, std::chrono::seconds(2),
                                 [&] { return mBlocks.size() >= blocks; });
    }
    bool advance(size_t periods) {
        size_t target;
        {
            std::lock_guard lock(mMutex);
            mReleased += periods;
            target = mReleased + 1;
            mChanged.notify_all();
        }
        return wait(target);
    }
    void release() {
        std::lock_guard lock(mMutex);
        mFree = true;
        mChanged.notify_all();
    }
    std::vector<int32_t> samples() {
        std::lock_guard lock(mMutex);
        std::vector<int32_t> result;
        for (const auto& b : mBlocks) result.insert(result.end(), b.begin(), b.end());
        return result;
    }
  private:
    std::mutex mMutex;
    std::condition_variable mChanged;
    size_t mReleased = 0;
    bool mFree = false;
    std::vector<std::vector<int32_t>> mBlocks;
};
class GatedOutput {
  public:
    GatedOutput() : output([this](auto b) { return sink.write(b); },
                           [] { return true; }, [] {}) {}
    ~GatedOutput() { sink.release(); } // Release before output joins its writer.
    GatedSink sink;
    ContinuousOutput output;
};
TEST(ContinuousOutput, AndroidBurstWaitsForSpaceAndPreservesEverySample) {
    GatedOutput test;
    ASSERT_TRUE(test.sink.wait(1));
    const auto client = test.output.attach();
    std::vector<int16_t> audio(8192 * 2);
    for (size_t i = 0; i < audio.size(); ++i) audio[i] = i + 1;
    ASSERT_EQ(4096u, test.output.enqueue(client, std::span(audio).first(8192)));
    ASSERT_TRUE(test.sink.advance(17)); // 4080 frames consumed, 16 remain.
    ASSERT_EQ(16u, test.output.queuedFrames(client));
    auto producer = std::async(std::launch::async, [&] {
        return test.output.enqueue(client, std::span(audio).subspan(8192));
    });
    EXPECT_EQ(std::future_status::timeout, producer.wait_for(std::chrono::milliseconds(20)));
    test.sink.release();
    EXPECT_EQ(4096u, producer.get());
    EXPECT_GT(test.output.statistics().producerWaits, 0u);
    EXPECT_EQ(0u, test.output.statistics().canceledFrames);
    ASSERT_TRUE(test.sink.wait(37));
    const auto actual = test.sink.samples();
    const auto begin = std::find_if(actual.begin(), actual.end(), [](int32_t x) { return x != 0; });
    ASSERT_GE(std::distance(begin, actual.end()), static_cast<ptrdiff_t>(audio.size()));
    for (size_t i = 0; i < audio.size(); ++i) {
        ASSERT_EQ(int32_t(audio[i]) * 65536, begin[i]) << "sample " << i;
    }
}
TEST(ContinuousOutput, FlushCancelsBlockedProducerWithoutReplayingItsTail) {
    GatedOutput test;
    ASSERT_TRUE(test.sink.wait(1));
    const auto client = test.output.attach();
    const std::vector<int16_t> old(4096 * 2, 11), tail(240 * 2, 22), fresh(240 * 2, 33);
    ASSERT_EQ(4096u, test.output.enqueue(client, old));
    auto producer = std::async(std::launch::async, [&] { return test.output.enqueue(client, tail); });
    EXPECT_EQ(std::future_status::timeout, producer.wait_for(std::chrono::milliseconds(20)));
    test.output.clear(client);
    EXPECT_EQ(0u, producer.get());
    EXPECT_EQ(0u, test.output.queuedFrames(client));
    EXPECT_EQ(4336u, test.output.statistics().canceledFrames);
    EXPECT_EQ(240u, test.output.enqueue(client, fresh));
    test.sink.release();
    ASSERT_TRUE(test.sink.wait(4));
    const auto samples = test.sink.samples();
    EXPECT_EQ(samples.end(), std::find(samples.begin(), samples.end(), 11 * 65536));
    EXPECT_EQ(samples.end(), std::find(samples.begin(), samples.end(), 22 * 65536));
    EXPECT_NE(samples.end(), std::find(samples.begin(), samples.end(), 33 * 65536));
}
TEST(ContinuousOutput, DetachWakesBlockedProducerAndReplacementKeepsClockRunning) {
    GatedOutput test;
    ASSERT_TRUE(test.sink.wait(1));
    const auto client = test.output.attach();
    const std::vector<int16_t> full(4096 * 2, 11), tail(240 * 2, 22);
    ASSERT_EQ(4096u, test.output.enqueue(client, full));
    auto producer = std::async(std::launch::async, [&] { return test.output.enqueue(client, tail); });
    EXPECT_EQ(std::future_status::timeout, producer.wait_for(std::chrono::milliseconds(20)));
    test.output.detach(client);
    EXPECT_EQ(0u, producer.get());
    const auto replacement = test.output.attach();
    EXPECT_NE(client, replacement);
    EXPECT_EQ(240u, test.output.enqueue(replacement, tail));
    test.sink.release();
    ASSERT_TRUE(test.sink.wait(4));
    EXPECT_TRUE(test.output.healthy());
    EXPECT_GT(test.output.framesWritten(), 0u);
}
TEST(ContinuousOutput, BurstLargerThanQueuePreservesAudioAcrossRepeatedWraps) {
    Sink sink;
    ContinuousOutput output([&](auto b) { return sink.write(b); }, [] { return true; }, [] {});
    ASSERT_TRUE(sink.wait(1));
    const auto client = output.attach();
    std::vector<int16_t> audio(4096 * 5 * 2);
    for (size_t i = 0; i < audio.size(); ++i) audio[i] = i % 20000 + 1;
    ASSERT_EQ(audio.size() / 2, output.enqueue(client, audio));
    ASSERT_TRUE(sink.wait(sink.count() + 20));
    std::vector<int32_t> actual;
    {
        std::lock_guard lock(sink.mutex);
        for (const auto& b : sink.blocks) actual.insert(actual.end(), b.begin(), b.end());
    }
    const auto begin = std::find_if(actual.begin(), actual.end(), [](int32_t x) { return x != 0; });
    ASSERT_GE(std::distance(begin, actual.end()), static_cast<ptrdiff_t>(audio.size()));
    for (size_t i = 0; i < audio.size(); ++i) {
        ASSERT_EQ(int32_t(audio[i]) * 65536, begin[i]) << "sample " << i;
    }
    EXPECT_GT(output.statistics().producerWaits, 0u);
    EXPECT_EQ(0u, output.statistics().canceledFrames);
}
TEST(ContinuousOutput, IdleDetachAndReplacementKeepSameWriterRunning) {
    Sink sink;
    ContinuousOutput output([&](auto b) { return sink.write(b); }, [] { return true; }, [] {});
    ASSERT_TRUE(sink.wait(4));
    EXPECT_TRUE(sink.zeroSince(0));
    auto first = output.attach();
    const std::vector<int16_t> audio(ContinuousOutput::kPeriodFrames * 2, -123);
    EXPECT_EQ(audio.size() / 2, output.enqueue(first, audio));
    ASSERT_TRUE(sink.wait(sink.count() + 4));
    EXPECT_TRUE(sink.contains(-123 * 65536));
    output.detach(first);  // Android standby/close.
    size_t idle = sink.count() + 1;  // Exclude a write already in flight.
    ASSERT_TRUE(sink.wait(idle + 6));
    EXPECT_TRUE(sink.zeroSince(idle));
    auto replacement = output.attach();
    EXPECT_NE(first, replacement);
    EXPECT_EQ(0u, output.enqueue(first, audio));  // Closed stream cannot leak samples.
    output.enqueue(replacement, audio);
    ASSERT_TRUE(sink.wait(sink.count() + 4));
    output.detach(replacement);
    ASSERT_TRUE(sink.wait(sink.count() + 6));
    EXPECT_EQ(0u, output.writeErrors());
}
TEST(ContinuousOutput, MutedFaultAndFlushDiscardAudioWithoutStoppingClock) {
    Sink sink;
    std::atomic<bool> audible = false;
    std::atomic<int> faults = 0;
    ContinuousOutput output([&](auto b) { return sink.write(b); },
                            [&] { return audible.load(); }, [&] { ++faults; });
    auto client = output.attach();
    const std::vector<int16_t> audio(ContinuousOutput::kPeriodFrames * 2, 1000);
    output.enqueue(client, audio);
    ASSERT_TRUE(sink.wait(5));
    EXPECT_TRUE(sink.zeroSince(0));
    EXPECT_EQ(0u, output.queuedFrames(client));
    sink.fail = true;
    ASSERT_TRUE(sink.wait(sink.count() + 4));
    EXPECT_GT(faults.load(), 0);
    sink.fail = false;
    output.enqueue(client, audio);
    output.clear(client);  // Flush/pause cannot stop the writer.
    audible = true;
    size_t resumed = sink.count() + 1;
    ASSERT_TRUE(sink.wait(resumed + 5));
    EXPECT_TRUE(sink.zeroSince(resumed));
    EXPECT_GT(output.framesWritten(), 0u);
}
}  // namespace
}  // namespace aidl::android::hardware::audio::core::a2b
