/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace aidl::android::hardware::ir {

// Renders an IR burst into the bitmap /dev/ir_spi clocks out to the LED.
//
// The driver sends the buffer as 32-bit SPI words, most significant bit first,
// one bit per clock: bit n of the burst is bit (31 - n % 32) of word n / 32.
// The result is padded with silence to the length the driver is given, and is
// empty if the burst is too short to hold a single bit. Returns nullopt if the
// pattern cannot be sent.
std::optional<std::vector<uint32_t>> RenderPattern(int32_t carrierFreqHz,
                                                   const std::vector<int32_t>& pattern);

}  // namespace aidl::android::hardware::ir
