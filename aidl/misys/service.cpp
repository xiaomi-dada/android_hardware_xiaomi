/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.xiaomi.hardware.misys.common-service.xiaomi"

#include "MiSys.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <sys/stat.h>

using aidl::vendor::xiaomi::hardware::misys::common::MiSys;

int main() {
    // Files and folders are read by the camera provider, which runs as another user
    umask(022);

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<MiSys> misys = ndk::SharedRefBase::make<MiSys>();
    const std::string instance = std::string(MiSys::descriptor) + "/default";
    binder_status_t status = AServiceManager_addService(misys->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK) << "Failed to add service " << instance << " " << status;

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
