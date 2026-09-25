// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

#include <json/json.h>

#include <string>

namespace aidl::android::hardware::audio::core::a2b {

// Shared by the HAL, offline validator and host tests. No Android dependencies.
struct Profile {
    Json::Value data;
    std::string id() const { return data["id"].asString(); }
};
bool validId(const std::string& id);
bool parseProfile(const std::string& text, Profile* profile, std::string* error);

class Transport {
   public:
    virtual ~Transport() = default;
    virtual bool write(int address, int reg, int value) = 0;
    virtual bool read(int address, int reg, int* value) = 0;
    virtual void delay(int milliseconds) = 0;
    virtual int64_t nowMs() const;
};

// Caller serializes ALL users of the main/bus addresses. Always clears PERI and
// broadcast selection, including on a failed peripheral access.
bool execute(const Profile& profile, const std::string& phase, Transport& transport,
             std::string* error);
bool verifyNodes(const Profile& profile, Transport& transport, std::string* error);

}  // namespace aidl::android::hardware::audio::core::a2b
