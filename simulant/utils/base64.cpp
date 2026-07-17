#include "base64.h"
#include <iostream>
#include <string>

namespace smlt {

optional<std::string> base64_decode(const std::string& encoded_string) {
    static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz"
                                            "0123456789+/";

    auto in_len = encoded_string.size();
    if(in_len == 0) {
        return std::string();
    }
    if(in_len % 4 != 0) {
        return no_value;
    }

    // '=' padding characters (up to two) may only appear at the very end.
    size_t padding = 0;
    if(encoded_string[in_len - 1] == '=') {
        padding++;
        if(encoded_string[in_len - 2] == '=') {
            padding++;
        }
    }

    std::string decoded_string;
    decoded_string.reserve((in_len / 4) * 3 - padding);

    for(size_t i = 0; i < in_len;) {
        uint32_t vals[4];
        for(int k = 0; k < 4; ++k) {
            char c = encoded_string[i++];
            if(c == '=') {
                vals[k] = 0;
            } else {
                auto pos = base64_chars.find(c);
                if(pos == std::string::npos) {
                    return no_value;
                }
                vals[k] = (uint32_t)pos;
            }
        }

        uint32_t triple = (vals[0] << 3 * 6) + (vals[1] << 2 * 6) +
                          (vals[2] << 1 * 6) + vals[3];
        decoded_string.push_back((triple >> 2 * 8) & 0xFF);
        decoded_string.push_back((triple >> 1 * 8) & 0xFF);
        decoded_string.push_back((triple >> 0 * 8) & 0xFF);
    }

    if(padding > 0) {
        decoded_string.resize(decoded_string.size() - padding);
    }

    return decoded_string;
}

} // namespace smlt
