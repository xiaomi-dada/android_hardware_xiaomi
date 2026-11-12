/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "android.hardware.ir-service.xiaomi"

#include "ConsumerIr.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::ir::ConsumerIr;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<ConsumerIr> ir = ndk::SharedRefBase::make<ConsumerIr>();
    const std::string instance = std::string(ConsumerIr::descriptor) + "/default";
    binder_status_t status = AServiceManager_addService(ir->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK) << "Failed to add service " << instance << " " << status;

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
