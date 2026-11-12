/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/ir/BnConsumerIr.h>

namespace aidl::android::hardware::ir {

// Drives the IR LED Xiaomi hangs off a SPI bus: bursts are rendered into a
// bitmap here and clocked out by the kernel through /dev/ir_spi.
class ConsumerIr : public BnConsumerIr {
  public:
    ndk::ScopedAStatus getCarrierFreqs(std::vector<ConsumerIrFreqRange>* _aidl_return) override;
    ndk::ScopedAStatus transmit(int32_t carrierFreqHz,
                                const std::vector<int32_t>& pattern) override;
};

}  // namespace aidl::android::hardware::ir
