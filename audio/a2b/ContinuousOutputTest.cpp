// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#include "ContinuousOutput.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>

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
