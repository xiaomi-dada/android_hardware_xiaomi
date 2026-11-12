/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SpiPattern.h"

namespace aidl::android::hardware::ir {

namespace {

// ir-spi.ko clocks the LED at this rate, one bit per clock.
constexpr uint64_t kSpiClockHz = 1'920'000;
constexpr uint64_t kUsPerSecond = 1'000'000;
constexpr uint32_t kBitsPerWord = 32;

// Every slice ends this much early. Xiaomi's HAL does this, and it is kept so
// that the bursts sent are the ones the stock HAL sends.
constexpr int64_t kEdgeAdvanceUs = 5;

// The longest burst Xiaomi's HAL sends: one more microsecond and its 32-bit
// count of clock cycles overflows.
constexpr int64_t kMaxDurationUs = 1'118'482;

// The LED is lit for the first 2/5 of each carrier period.
constexpr uint32_t kDutyCycleNum = 2;
constexpr uint32_t kDutyCycleDen = 5;

// The burst is padded with silence to a multiple of this many bytes.
constexpr size_t kLengthAlignment = 1024;

}  // namespace

std::optional<std::vector<uint32_t>> RenderPattern(int32_t carrierFreqHz,
                                                   const std::vector<int32_t>& pattern) {
    if (carrierFreqHz <= 0 || pattern.empty()) {
        return std::nullopt;
    }

    // When each slice ends, in microseconds from the start of the burst.
    std::vector<int64_t> ends;
    ends.reserve(pattern.size());

    int64_t end = -kEdgeAdvanceUs;
    for (int32_t duration : pattern) {
        if (duration <= 0) {
            return std::nullopt;
        }
        end += duration;
        ends.push_back(end);
    }

    if (end < 0 || end >= kMaxDurationUs) {
        return std::nullopt;
    }

    const uint64_t bits = end * kSpiClockHz / kUsPerSecond;
    size_t bytes = (bits + 7) / 8;
    bytes = (bytes + kLengthAlignment - 1) / kLengthAlignment * kLengthAlignment;
    std::vector<uint32_t> words(bytes / sizeof(uint32_t));

    const uint32_t frequency = carrierFreqHz;
    const uint32_t period = (kSpiClockHz + frequency / 2) / frequency;
    const uint32_t onTime = period * kDutyCycleNum / kDutyCycleDen;

    // Every mark starts its carrier from the beginning of a period.
    uint32_t phase = 0;
    size_t slice = 0;

    for (uint64_t bit = 0; bit < bits; bit++) {
        const int64_t time = bit * kUsPerSecond / kSpiClockHz;
        while (slice < ends.size() && time > ends[slice]) {
            slice++;
        }
        if (slice == ends.size()) {
            break;
        }

        // Even slices are marks, odd ones spaces.
        if (slice % 2) {
            phase = 0;
            continue;
        }

        if (phase < onTime) {
            words[bit / kBitsPerWord] |= 1u << (kBitsPerWord - 1 - bit % kBitsPerWord);
        }
        if (++phase == period) {
            phase = 0;
        }
    }

    return words;
}

}  // namespace aidl::android::hardware::ir
