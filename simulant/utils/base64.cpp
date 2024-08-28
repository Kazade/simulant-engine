#include "base64.h"
#include <iostream>
#include <string>

namespace smlt {

optional<std::string> base64_decode(const std::string& encoded_string) {
    static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz"
                                            "0123456789+/";

    // Remove any padding characters
    size_t padding = 0;
    auto in_len = encoded_string.size();
    size_t out_len = (in_len * 3) / 4 - padding;
    std::string decoded_string;
    decoded_string.reserve(out_len);

    if(in_len >= 2 && encoded_string[in_len - 1] == '=') {
        padding++;
        if(encoded_string[in_len - 2] == '=') {
            padding++;
        }
    }

    for(size_t i = 0; i < in_len;) {
        uint32_t a = base64_chars.find(encoded_string[i++]);
        uint32_t b = (i < in_len) ? base64_chars.find(encoded_string[i++]) : 0;
        uint32_t c = (i < in_len) ? base64_chars.find(encoded_string[i++]) : 0;
        uint32_t d = (i < in_len) ? base64_chars.find(encoded_string[i++]) : 0;

        if(a == std::string::npos || b == std::string::npos ||
           c == std::string::npos || d == std::string::npos) {
            return no_value;
        }

        uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + d;
        decoded_string.push_back((triple >> 2 * 8) & 0xFF);
        decoded_string.push_back((triple >> 1 * 8) & 0xFF);
        decoded_string.push_back((triple >> 0 * 8) & 0xFF);
    }

    return decoded_string;
}

} // namespace smlt
