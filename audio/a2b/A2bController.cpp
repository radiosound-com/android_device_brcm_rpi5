/*
 * Copyright 2026 Radio Sound, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "AHAL_A2B"

#include "a2b/A2bController.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <android-base/logging.h>

#include "a2b/A2bRoutines.h"

namespace aidl::android::hardware::audio::core::a2b {
namespace {

constexpr char kI2cDevice[] = "/dev/i2c-1";

class I2cSmbusTransport final {
  public:
    ~I2cSmbusTransport() {
        if (mFd >= 0) {
            close(mFd);
        }
    }

    ::android::status_t open() {
        mFd = ::open(kI2cDevice, O_RDWR | O_CLOEXEC);
        if (mFd < 0) {
            LOG(ERROR) << "A2B: cannot open " << kI2cDevice << ": " << strerror(errno);
            return ::android::NO_INIT;
        }
        return ::android::OK;
    }

    ::android::status_t writeByteData(uint8_t deviceAddress, uint8_t registerAddress,
                                      uint8_t value) {
        union i2c_smbus_data data = {};
        data.byte = value;
        return transfer(deviceAddress, registerAddress, I2C_SMBUS_WRITE, data);
    }

    ::android::status_t readByteData(uint8_t deviceAddress, uint8_t registerAddress,
                                     uint8_t* value) {
        union i2c_smbus_data data = {};
        const ::android::status_t status =
                transfer(deviceAddress, registerAddress, I2C_SMBUS_READ, data);
        if (status == ::android::OK) {
            *value = data.byte;
        }
        return status;
    }

  private:
    ::android::status_t transfer(uint8_t deviceAddress, uint8_t registerAddress,
                                 uint8_t direction, union i2c_smbus_data& data) {
        if (mFd < 0 || deviceAddress == 0) {
            return ::android::NO_INIT;
        }

        if (ioctl(mFd, I2C_SLAVE, deviceAddress) < 0) {
            LOG(ERROR) << "A2B: cannot select I2C address 0x" << std::hex
                       << static_cast<int>(deviceAddress) << ": " << strerror(errno);
            return ::android::UNKNOWN_ERROR;
        }

        struct i2c_smbus_ioctl_data args = {};
        args.read_write = direction;
        args.command = registerAddress;
        args.size = I2C_SMBUS_BYTE_DATA;
        args.data = &data;
        if (ioctl(mFd, I2C_SMBUS, &args) < 0) {
            LOG(ERROR) << "A2B: SMBus " << (direction == I2C_SMBUS_READ ? "read" : "write")
                       << " failed at 0x" << std::hex << static_cast<int>(deviceAddress)
                       << ":0x" << static_cast<int>(registerAddress) << ": " << strerror(errno);
            return ::android::UNKNOWN_ERROR;
        }
        return ::android::OK;
    }

    int mFd = -1;
};

::android::status_t executeRoutine(const A2bRoutine& routine) {
    I2cSmbusTransport transport;
    if (const ::android::status_t status = transport.open(); status != ::android::OK) {
        return status;
    }

    LOG(INFO) << "A2B: applying " << routine.name << " (" << routine.operationCount
              << " operations)";

    for (size_t index = 0; index < routine.operationCount; ++index) {
        const A2bOperation& operation = routine.operations[index];
        ::android::status_t status = ::android::OK;
        switch (operation.type) {
            case A2bOperationType::Write:
                status = transport.writeByteData(operation.deviceAddress,
                                                  operation.registerAddress, operation.value);
                break;
            case A2bOperationType::Read: {
                uint8_t value = 0;
                status = transport.readByteData(operation.deviceAddress,
                                                operation.registerAddress, &value);
                if (status == ::android::OK) {
                    LOG(VERBOSE) << "A2B: read 0x" << std::hex
                                 << static_cast<int>(operation.deviceAddress) << ":0x"
                                 << static_cast<int>(operation.registerAddress) << " = 0x"
                                 << static_cast<int>(value);
                }
                break;
            }
            case A2bOperationType::Delay:
                usleep(static_cast<useconds_t>(operation.value) * 1000U);
                break;
        }

        if (status != ::android::OK) {
            LOG(ERROR) << "A2B: " << routine.name << " failed at operation " << index;
            return status;
        }
    }

    LOG(INFO) << "A2B: " << routine.name << " is ready";
    return ::android::OK;
}

}  // namespace

A2bController& A2bController::getInstance() {
    static A2bController instance;
    return instance;
}

::android::status_t A2bController::initialize() {
    std::lock_guard lock(mLock);
    if (mInitialized) {
        return ::android::OK;
    }

    const ::android::status_t status = executeRoutine(getSelectedRoutine());
    if (status == ::android::OK) {
        mInitialized = true;
    }
    return status;
}

}  // namespace aidl::android::hardware::audio::core::a2b
