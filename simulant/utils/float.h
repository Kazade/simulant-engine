#pragma once

#include <cstdint>

namespace smlt {

struct Float10 {
    static constexpr int max_value = 64512;
    union {
        struct {
            unsigned value : 10;
            unsigned unused : 6;
        } f;
        uint16_t i = 0;
    };
};

optional<Float10> float10_from_float(float input) {
    // Check if the input is outside the representable range
    if(input < 0.0f || input > Float10::max_value || std::isinf(input) ||
       std::isnan(input)) {
        return optional<Float10>();
    }

    Float10 result;
    if(input == 0.0f) {
        result.i = 0;
        return result;
    }

    // Extract the exponent and mantissa from the 32-bit float
    uint32_t float_bits = *reinterpret_cast<uint32_t*>(&input);
    uint32_t exponent = (float_bits >> 23) & 0xFF; // 8 bits
    uint32_t mantissa = float_bits & 0x7FFFFF;     // 23 bits

    // Normalize the exponent to fit into 5 bits
    int32_t biased_exponent = exponent - 127; // Unbias the exponent
    int32_t new_exponent =
        biased_exponent + 15; // Bias for 5 bits (15 is the bias for 5 bits)

    // Check for overflow/underflow
    if(new_exponent >= 31) {
        S_ERROR("Exponent overflow");
        return optional<Float10>();
    }

    if(new_exponent < 0) {
        S_ERROR("Exponent underflow");
        return optional<Float10>();
    }

    // Normalize the mantissa to fit into 5 bits
    mantissa >>= 18; // Shift right to fit into 5 bits (23 - 5 = 18)

    // Combine exponent and mantissa into the 10-bit representation
    result.i = (new_exponent << 5) | (mantissa & 0x1F); // 0x1F is 5 bits mask

    return result;
}

float float10_to_float(Float10 f) {
    // Extract the exponent and mantissa from the 10-bit representation
    uint32_t exponent = (f.i >> 5) & 0x1F; // 5 bits for exponent
    uint32_t mantissa = f.i & 0x1F;        // 5 bits for mantissa

    // Calculate the actual exponent
    int32_t actualExponent = exponent - 15; // Unbias the exponent

    // Calculate the effective mantissa
    float effectiveMantissa = 1.0f + (static_cast<float>(mantissa) / 32.0f);

    // Calculate the final float value
    float result = effectiveMantissa * std::powf(2.0f, actualExponent);

    return result;
}

} // namespace smlt
