// Copyright 2026 Radio Sound, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "BootControl.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::boot::BootControl;
using aidl::android::hardware::boot::IBootControl;

int main(int argc, char** argv) {
    android::base::InitLogging(argv, android::base::KernelLogger);
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<BootControl>();
    const std::string name = std::string(IBootControl::descriptor) + "/default";
    CHECK_EQ(AServiceManager_addService(service->asBinder().get(), name.c_str()), STATUS_OK);
    ABinderProcess_joinThreadPool();
    return 1;
}
