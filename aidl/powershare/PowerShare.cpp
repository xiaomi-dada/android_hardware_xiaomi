/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.lineage.powershare-service.xiaomi"

#include "PowerShare.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>

using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

namespace {

// The charger driver refuses to transmit while the phone is charging itself
// and turns transmission off on its own, so the mode is read back each time.
constexpr const char* kReverseChargeModePath = "/sys/class/qcom-battery/reverse_chg_mode";

}  // anonymous namespace

namespace aidl {
namespace vendor {
namespace lineage {
namespace powershare {

ndk::ScopedAStatus PowerShare::getMinBattery(int32_t* _aidl_return) {
    *_aidl_return = 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerShare::isEnabled(bool* _aidl_return) {
    std::string value;
    if (!ReadFileToString(kReverseChargeModePath, &value)) {
        PLOG(ERROR) << "Failed to read " << kReverseChargeModePath;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    *_aidl_return = Trim(value) == "1";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerShare::setEnabled(bool enable) {
    if (!WriteStringToFile(enable ? "1" : "0", kReverseChargeModePath)) {
        PLOG(ERROR) << "Failed to write " << kReverseChargeModePath;
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerShare::setMinBattery(int32_t /* minBattery */) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace powershare
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
