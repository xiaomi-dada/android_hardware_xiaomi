/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "android.hardware.ir-service.xiaomi"

#include "ConsumerIr.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "SpiPattern.h"

using android::base::unique_fd;

namespace aidl::android::hardware::ir {

namespace {

constexpr char kIrDevice[] = "/dev/ir_spi";

// Sets the length in bytes of the burst the next write() carries.
constexpr unsigned long kIrSpiSetLength = _IOW('i', 0x11, uint32_t);

// What Xiaomi's HAL advertises. Any other carrier is sent too, as closely as
// the SPI clock divides down to it.
const std::vector<ConsumerIrFreqRange> kCarrierFreqs = {
        {.minHz = 30000, .maxHz = 30000}, {.minHz = 33000, .maxHz = 33000},
        {.minHz = 36000, .maxHz = 36000}, {.minHz = 38000, .maxHz = 38000},
        {.minHz = 40000, .maxHz = 40000}, {.minHz = 56000, .maxHz = 56000},
};

}  // namespace

ndk::ScopedAStatus ConsumerIr::getCarrierFreqs(std::vector<ConsumerIrFreqRange>* _aidl_return) {
    *_aidl_return = kCarrierFreqs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ConsumerIr::transmit(int32_t carrierFreqHz,
                                        const std::vector<int32_t>& pattern) {
    if (carrierFreqHz <= 0) {
        LOG(ERROR) << "Unsupported carrier " << carrierFreqHz;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    auto bitmap = RenderPattern(carrierFreqHz, pattern);
    if (!bitmap) {
        LOG(ERROR) << "Cannot send a pattern of " << pattern.size() << " entries";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (bitmap->empty()) {
        return ndk::ScopedAStatus::ok();
    }

    // The driver forgets the burst length when the last user closes it, and
    // resizing it while it is set is not safe, so every burst opens afresh.
    unique_fd fd(TEMP_FAILURE_RETRY(open(kIrDevice, O_WRONLY | O_CLOEXEC)));
    if (!fd.ok()) {
        PLOG(ERROR) << "Failed to open " << kIrDevice;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    uint32_t length = bitmap->size() * sizeof(uint32_t);
    if (ioctl(fd.get(), kIrSpiSetLength, &length) < 0) {
        PLOG(ERROR) << "Failed to set the burst length to " << length;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    // Returns once the burst has been clocked out.
    ssize_t written = TEMP_FAILURE_RETRY(write(fd.get(), bitmap->data(), length));
    if (written != static_cast<ssize_t>(length)) {
        PLOG(ERROR) << "Failed to send " << length << " bytes, sent " << written;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::ir
