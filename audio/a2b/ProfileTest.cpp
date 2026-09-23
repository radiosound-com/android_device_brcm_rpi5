// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <map>
#include <tuple>
#include <vector>

#include "Profile.h"

namespace aidl::android::hardware::audio::core::a2b {
namespace {
Profile fixture() {
    Profile p;
    p.data = Json::Value(Json::objectValue);
    p.data["version"] = 1;
    p.data["id"] = "test";
    p.data["description"] = "fixture";
    p.data["i2c_bus"] = 1;
    p.data["main_address"] = 104;
    p.data["sample_rate"] = 48000;
    p.data["channels"] = 2;
    p.data["start_muted"] = true;
    p.data["nodes"][0]["vendor"] = 173;
    p.data["nodes"][0]["product"] = 40;
    p.data["peripherals"]["amp"]["node"] = 0;
    p.data["peripherals"]["amp"]["address"] = 108;
    Json::Value o;
    o["op"] = "verify";
    o["target"] = "amp";
    o["reg"] = 2;
    o["mask"] = 255;
    o["value"] = 4;
    for (auto phase : {"init", "mute", "unmute", "shutdown", "health"})
        p.data["sequences"][phase].append(o);
    return p;
}
bool validate(const Profile& p) {
    Profile parsed;
    std::string error;
    return parseProfile(Json::writeString(Json::StreamWriterBuilder(), p.data), &parsed, &error);
}

class FakeBus : public Transport {
   public:
    int selection = 0, chip = 0, elapsed = 0, failReg = -1, resetWait = 0;
    bool discovery = true;
    std::map<std::tuple<int, int, int>, int> registers;
    std::vector<std::tuple<int, int, int>> writes;
    FakeBus() {
        registers[{1, 0, 2}] = 173;
        registers[{1, 0, 3}] = 40;
        registers[{2, 108, 1}] = 0xfd;
        registers[{2, 108, 2}] = 4;
        registers[{2, 108, 3}] = 0x80;
    }
    auto key(int address, int reg) {
        return std::make_tuple(address == 104     ? 0
                               : selection & 0x20 ? 2
                                                  : 1,
                               address == 104     ? 0
                               : selection & 0x20 ? chip
                                                  : selection,
                               reg);
    }
    bool write(int address, int reg, int value) override {
        writes.emplace_back(address, reg, value);
        if (resetWait > 0) return false;
        if (address == 104 && reg == 0x12 && value == 0x84) resetWait = 25;
        if (address == 104 && reg == 1) {
            selection = value;
            return true;
        }
        if (address == 105 && !(selection & 0x20) && reg == 0) {
            chip = value;
            return true;
        }
        if (address == 105 && (selection & 0x20) && reg == failReg) return false;
        if (address == 104 && reg == 0x13) registers[{0, 0, 0x1a}] = discovery ? 1 : 0;
        registers[key(address, reg)] = value;
        return true;
    }
    bool read(int address, int reg, int* value) override {
        *value = registers[key(address, reg)];
        return true;
    }
    void delay(int ms) override {
        elapsed += ms;
        resetWait -= ms;
    }
    int64_t nowMs() const override { return elapsed; }
};

TEST(Profile, RejectsTyposRangesAndRoutingEscape) {
    const auto good = fixture();
    ASSERT_TRUE(validate(good));
    for (const auto& id : {"../test", "a/b", "", "Test"}) {
        auto p = good;
        p.data["id"] = id;
        EXPECT_FALSE(validate(p));
    }
    auto p = good;
    p.data["sample_rate"] = 44100;
    EXPECT_FALSE(validate(p));
    p = good;
    p.data["nodes"] = Json::Value(Json::arrayValue);
    EXPECT_FALSE(validate(p));
    p = good;
    p.data["peripherals"]["amp"]["node"] = 1;
    EXPECT_FALSE(validate(p));
    p = good;
    p.data["sequences"]["init"][0]["value"] = 256;
    EXPECT_FALSE(validate(p));
    p = good;
    p.data["sequences"]["init"][0]["typo"] = 1;
    EXPECT_FALSE(validate(p));
    p = good;
    p.data["sequences"]["init"][0] = 42;
    EXPECT_FALSE(validate(p));
    p = good;
    auto& o = p.data["sequences"]["init"][0];
    o["op"] = "update";
    o["target"] = "main";
    o["reg"] = 1;
    EXPECT_FALSE(validate(p));
    o["target"] = "node0";
    o["reg"] = 0;
    EXPECT_FALSE(validate(p));
    p = good;
    p.data["sequences"]["health"][0]["op"] = "update";
    EXPECT_FALSE(validate(p));
    std::string error;
    Profile out;
    EXPECT_FALSE(parseProfile("{\"version\":1,\"version\":2}", &out, &error));
    EXPECT_FALSE(parseProfile(std::string(262145, ' '), &out, &error));
    EXPECT_FALSE(parseProfile(std::string(40, '[') + "0" + std::string(40, ']'), &out, &error));
}

TEST(Executor, SelectsPeripheralAndPreservesClipperBits) {
    auto p = fixture();
    auto& o = p.data["sequences"]["init"][0];
    o["op"] = "update";
    o["reg"] = 1;
    o["mask"] = 3;
    o["value"] = 2;
    FakeBus bus;
    std::string error;
    ASSERT_TRUE(execute(p, "init", bus, &error)) << error;
    EXPECT_EQ(0xfe, bus.registers[std::make_tuple(2, 108, 1)]);
    EXPECT_EQ(0, bus.selection);
    const std::vector<std::tuple<int, int, int>> expected = {
        {104, 1, 0}, {105, 0, 108}, {104, 1, 0x20}, {105, 1, 0xfe}, {104, 1, 0}};
    EXPECT_EQ(expected, bus.writes);
    bus.failReg = 1;
    EXPECT_FALSE(execute(p, "init", bus, &error));
    EXPECT_EQ(0, bus.selection);  // PERI cleared even on failure.
}

TEST(Executor, PollTimeoutAndIdentityMismatchFailClosed) {
    auto p = fixture();
    auto& o = p.data["sequences"]["init"][0];
    o["op"] = "poll";
    o["timeout_ms"] = 12;
    o["value"] = 9;
    FakeBus bus;
    std::string error;
    EXPECT_FALSE(execute(p, "init", bus, &error));
    EXPECT_EQ(12, bus.elapsed);
    EXPECT_EQ(0, bus.selection);
    ASSERT_TRUE(verifyNodes(p, bus, &error));
    bus.registers[{1, 0, 3}] = 0x27;
    EXPECT_FALSE(verifyNodes(p, bus, &error));
    EXPECT_EQ(0, bus.selection);
}

TEST(Executor, ShutdownAttemptsGpioAfterAmplifierFailure) {
    auto p = fixture();
    auto& sequence = p.data["sequences"]["shutdown"];
    sequence[0]["op"] = "update";
    sequence[0]["reg"] = 1;
    sequence[0]["mask"] = 3;
    sequence[0]["value"] = 2;
    Json::Value gpio;
    gpio["op"] = "write";
    gpio["target"] = "node0";
    gpio["reg"] = 0x4c;
    gpio["value"] = 16;
    sequence.append(gpio);
    FakeBus bus;
    bus.failReg = 1;
    std::string error;
    EXPECT_FALSE(execute(p, "shutdown", bus, &error));
    EXPECT_EQ(16, bus.registers[std::make_tuple(1, 0, 0x4c)]);
    EXPECT_EQ(0, bus.selection);
}

TEST(BundledProfile, TasLifecycleAndDiscoveryFailure) {
    const char* directory = std::getenv("A2B_PROFILE_DIR");
    ASSERT_NE(nullptr, directory) << "Set A2B_PROFILE_DIR to audio/a2b/profiles";
    std::ifstream file(std::string(directory) + "/tas5720a_1node.json");
    ASSERT_TRUE(file.good());
    const std::string text((std::istreambuf_iterator<char>(file)), {});
    Profile p;
    std::string error;
    ASSERT_TRUE(parseProfile(text, &p, &error)) << error;
    FakeBus bus;
    ASSERT_TRUE(execute(p, "init", bus, &error)) << error;
    ASSERT_TRUE(verifyNodes(p, bus, &error)) << error;
    ASSERT_TRUE(execute(p, "health", bus, &error)) << error;
    EXPECT_EQ(0x81, bus.registers[std::make_tuple(1, 0, 0x5a)]);
    EXPECT_EQ(0x9b, bus.registers[std::make_tuple(2, 108, 6)]);
    ASSERT_TRUE(execute(p, "unmute", bus, &error));
    EXPECT_EQ(0x80, bus.registers[std::make_tuple(2, 108, 3)]);
    ASSERT_TRUE(execute(p, "mute", bus, &error));
    ASSERT_TRUE(execute(p, "shutdown", bus, &error));
    EXPECT_EQ(0x83, bus.registers[std::make_tuple(2, 108, 3)]);
    EXPECT_EQ(0xfe, bus.registers[std::make_tuple(2, 108, 1)]);
    FakeBus broken;
    broken.discovery = false;
    EXPECT_FALSE(execute(p, "init", broken, &error));
    EXPECT_EQ(0, broken.selection);
    // Failed discovery must never configure/unmute TAS.
    EXPECT_EQ(0xfd, broken.registers[std::make_tuple(2, 108, 1)]);
}
}  // namespace
}  // namespace aidl::android::hardware::audio::core::a2b
