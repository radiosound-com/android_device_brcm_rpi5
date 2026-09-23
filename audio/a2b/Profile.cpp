// Copyright 2026 Radio Sound, Inc. SPDX-License-Identifier: Apache-2.0
#include "Profile.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string_view>

namespace aidl::android::hardware::audio::core::a2b {
namespace {
bool fields(const Json::Value& v, std::initializer_list<const char*> allowed) {
    if (!v.isObject()) return false;
    for (const auto& key : v.getMemberNames()) {
        if (std::none_of(allowed.begin(), allowed.end(), [&](auto a) { return key == a; }))
            return false;
    }
    return true;
}
bool number(const Json::Value& v, int lo, int hi) {
    return v.isInt() && v.asInt() >= lo && v.asInt() <= hi;
}
bool fail(std::string* error, const std::string& message) {
    *error = message;
    return false;
}
int nodeIndex(const Profile& p, const std::string& target) {
    for (Json::ArrayIndex i = 0; i < p.data["nodes"].size(); ++i)
        if (target == "node" + std::to_string(i)) return i;
    return -1;
}
bool select(const Profile& p, const std::string& target, Transport& t, int* address) {
    const int main = p.data["main_address"].asInt();
    *address = main;
    if (target == "main") return true;
    *address = main + 1;
    const int node = nodeIndex(p, target);
    if (node >= 0) return t.write(main, 1, node);
    const auto& peripheral = p.data["peripherals"][target];
    const int peripheralNode = peripheral["node"].asInt();
    return t.write(main, 1, peripheralNode) &&
           t.write(main + 1, 0, peripheral["address"].asInt()) &&
           t.write(main, 1, 0x20 | peripheralNode);
}
}  // namespace

int64_t Transport::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool validId(const std::string& id) {
    return !id.empty() && id.size() <= 64 &&
           id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") == std::string::npos;
}

bool parseProfile(const std::string& text, Profile* profile, std::string* error) {
    if (text.size() > 262144) return fail(error, "profile exceeds 256 KiB");
    // JsonCpp aborts on its stack limit in exception-free Android builds.
    // Reject excessive nesting before entering the parser, respecting strings.
    int depth = 0;
    bool quoted = false, escaped = false;
    for (char ch : text) {
        if (quoted) {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                quoted = false;
        } else if (ch == '"')
            quoted = true;
        else if (ch == '{' || ch == '[') {
            if (++depth > 16) return fail(error, "profile nesting exceeds 16 levels");
        } else if ((ch == '}' || ch == ']') && --depth < 0) {
            return fail(error, "unbalanced JSON");
        }
    }
    Json::CharReaderBuilder builder;
    builder["rejectDupKeys"] = true;
    builder["failIfExtra"] = true;
    builder["allowComments"] = false;
    builder["stackLimit"] = 32;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Profile p;
    if (!reader->parse(text.data(), text.data() + text.size(), &p.data, error)) return false;
    const auto& d = p.data;
    if (!fields(d, {"version", "id", "description", "i2c_bus", "main_address", "nodes",
                    "peripherals", "sample_rate", "channels", "start_muted", "sequences"}) ||
        d["version"] != 1 || !d["id"].isString() || !validId(p.id()) ||
        !d["description"].isString() || !number(d["i2c_bus"], 0, 255) ||
        !number(d["main_address"], 0x68, 0x6a) || d["main_address"].asInt() % 2 != 0 ||
        d["sample_rate"] != 48000 || d["channels"] != 2 || !d["start_muted"].isBool())
        return fail(error, "invalid header (version 1, 48 kHz stereo required)");
    if (!d["nodes"].isArray() || d["nodes"].empty() || d["nodes"].size() > 10)
        return fail(error, "nodes must describe 1..10 physical positions");
    for (const auto& n : d["nodes"]) {
        if (!fields(n, {"vendor", "product"}) || !number(n["vendor"], 0, 255) ||
            !number(n["product"], 0, 255))
            return fail(error, "invalid node identity");
    }
    if (!d["peripherals"].isObject()) return fail(error, "peripherals must be an object");
    for (const auto& name : d["peripherals"].getMemberNames()) {
        const auto& v = d["peripherals"][name];
        if (!validId(name) || name == "main" || name.starts_with("node") ||
            !fields(v, {"node", "address"}) || !number(v["node"], 0, d["nodes"].size() - 1) ||
            !number(v["address"], 8, 119))
            return fail(error, "invalid peripheral " + name);
    }
    const auto& sequences = d["sequences"];
    if (!fields(sequences, {"init", "mute", "unmute", "shutdown", "health"}))
        return fail(error, "unknown sequence");
    size_t count = 0;
    for (auto phase : {"init", "mute", "unmute", "shutdown", "health"}) {
        const auto& ops = sequences[phase];
        if (!ops.isArray() || ops.empty()) return fail(error, std::string("missing ") + phase);
        int budget = 0;
        for (const auto& o : ops) {
            if (++count > 2048 || !o.isObject() || !o["op"].isString())
                return fail(error, "invalid operation count/type");
            const std::string op = o["op"].asString();
            if (op == "delay") {
                if (!fields(o, {"op", "ms", "note"}) || !number(o["ms"], 1, 500) ||
                    std::string_view(phase) == "health")
                    return fail(error, "invalid delay");
                budget += o["ms"].asInt();
            } else {
                if (!fields(o, {"op", "target", "reg", "value", "mask", "timeout_ms", "note"}) ||
                    !o["target"].isString() || !number(o["reg"], 0, 255))
                    return fail(error, "invalid register operation");
                const auto target = o["target"].asString();
                const bool isMain = target == "main", isNode = nodeIndex(p, target) >= 0;
                if (!isMain && !isNode && !d["peripherals"].isMember(target))
                    return fail(error, "unknown target " + target);
                if (op != "write" && op != "update" && op != "verify" && op != "poll")
                    return fail(error, "unknown operation " + op);
                if (std::string_view(phase) == "health" && op != "verify")
                    return fail(error, "health permits only verification");
                if (!number(o["value"], 0, 255)) return fail(error, "invalid value");
                if (op != "write" &&
                    (!number(o["mask"], 1, 255) || (o["value"].asInt() & ~o["mask"].asInt())))
                    return fail(error, "value must fit nonzero mask");
                if ((op == "write" && o.isMember("mask")) ||
                    (op != "poll" && o.isMember("timeout_ms")))
                    return fail(error, "unused field");
                if ((op == "write" || op == "update") &&
                    ((isMain && o["reg"] == 1) || (isNode && o["reg"] == 0)))
                    return fail(error, "NODEADR/CHIP routing is owned by the executor");
                if (op == "poll") {
                    if (!number(o["timeout_ms"], 1, 1000))
                        return fail(error, "invalid poll timeout");
                    budget += o["timeout_ms"].asInt();
                }
            }
            if (o.isMember("note") && !o["note"].isString()) return fail(error, "invalid note");
            if (budget > 10000) return fail(error, "sequence exceeds 10 second wait budget");
        }
    }
    *profile = std::move(p);
    return true;
}

bool execute(const Profile& p, const std::string& phase, Transport& t, std::string* error) {
    const int main = p.data["main_address"].asInt();
    bool ok = true;
    bool allOk = true;
    size_t index = 0;
    for (const auto& o : p.data["sequences"][phase]) {
        ok = true;
        const auto op = o["op"].asString();
        if (op == "delay") {
            t.delay(o["ms"].asInt());
        } else {
            int address = 0, value = 0;
            ok = select(p, o["target"].asString(), t, &address);
            const int reg = o["reg"].asInt(), wanted = o["value"].asInt();
            const int mask = o["mask"].asInt();
            if (ok && op == "write")
                ok = t.write(address, reg, wanted);
            else if (ok && op == "update")
                ok =
                    t.read(address, reg, &value) && t.write(address, reg, (value & ~mask) | wanted);
            else if (ok) {
                const int64_t deadline = t.nowMs() + o["timeout_ms"].asInt();
                do {
                    ok = t.read(address, reg, &value);
                    if (!ok || (value & mask) == wanted) break;
                    const int64_t remaining = deadline - t.nowMs();
                    if (op != "poll" || remaining <= 0) {
                        ok = false;
                        break;
                    }
                    const int step = std::min<int64_t>(5, remaining);
                    t.delay(step);
                } while (true);
            }
        }
        // PERI must never leak into a later command, even after an I2C error.
        // Do not insert I2C traffic between main-node reset and its settling
        // delay. Only remote operations alter routing and require cleanup.
        const bool restored = op == "delay" || o["target"] == "main" || t.write(main, 1, 0);
        if (!ok || !restored) {
            if (allOk) fail(error, phase + " operation " + std::to_string(index) + " failed");
            allOk = false;
            // Independent GPIO shutdown must still be attempted if the amp NACKs.
            if (phase != "shutdown" && phase != "mute") break;
        }
        ++index;
    }
    return allOk;
}

bool verifyNodes(const Profile& p, Transport& t, std::string* error) {
    const int main = p.data["main_address"].asInt();
    bool ok = true;
    for (Json::ArrayIndex i = 0; i < p.data["nodes"].size(); ++i) {
        int vendor = 0, product = 0;
        ok = t.write(main, 1, i) && t.read(main + 1, 2, &vendor) && t.read(main + 1, 3, &product) &&
             vendor == p.data["nodes"][i]["vendor"].asInt() &&
             product == p.data["nodes"][i]["product"].asInt();
        if (!ok) {
            fail(error, "node " + std::to_string(i) + " identity mismatch/unreachable");
            break;
        }
    }
    const bool restored = t.write(main, 1, 0);
    if (!restored) return fail(error, "cannot clear NODEADR");
    return ok;
}
}  // namespace aidl::android::hardware::audio::core::a2b
