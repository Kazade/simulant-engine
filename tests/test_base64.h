#pragma once

#include <simulant/test.h>
#include <simulant/utils/base64.h>

namespace {

using namespace smlt;

class Base64Tests: public smlt::test::SimulantTestCase {
public:
    // Regression test for a bug where any base64 string with '=' padding
    // failed to decode (the padding characters were run through the
    // alphabet lookup instead of being treated as padding), which broke
    // decoding of essentially every real-world payload - most notably the
    // embedded .dtex textures produced by tools/optimise_gltf.
    void test_decode_with_one_padding_char() {
        auto result = base64_decode("TWE=");
        assert_true(bool(result));
        assert_equal(std::string("Ma"), *result);
    }

    void test_decode_with_two_padding_chars() {
        auto result = base64_decode("TQ==");
        assert_true(bool(result));
        assert_equal(std::string("M"), *result);
    }

    void test_decode_with_no_padding() {
        auto result = base64_decode("TWFu");
        assert_true(bool(result));
        assert_equal(std::string("Man"), *result);
    }

    void test_decode_empty_string() {
        auto result = base64_decode("");
        assert_true(bool(result));
        assert_equal(std::string(), *result);
    }

    void test_decode_longer_payload_with_padding() {
        // "Hello, world!" is 13 bytes -> not a multiple of 3, so this
        // exercises single-padding decoding on a longer input.
        auto result = base64_decode("SGVsbG8sIHdvcmxkIQ==");
        assert_true(bool(result));
        assert_equal(std::string("Hello, world!"), *result);
    }

    void test_decode_rejects_invalid_length() {
        // Valid base64 is always a multiple of 4 characters.
        auto result = base64_decode("TWE");
        assert_false(bool(result));
    }

    void test_decode_rejects_invalid_character() {
        auto result = base64_decode("TW$=");
        assert_false(bool(result));
    }

    void test_decode_matches_encoded_binary_round_trip() {
        // Every possible padding remainder (0, 1, 2 bytes) with non-text
        // bytes, to make sure padding handling doesn't depend on the data
        // being valid ASCII/UTF-8.
        std::string three_bytes(3, '\x01');
        std::string two_bytes(2, '\x02');
        std::string one_byte(1, '\x03');

        // Precomputed base64 for the byte strings above.
        auto r3 = base64_decode("AQEB");
        auto r2 = base64_decode("AgI=");
        auto r1 = base64_decode("Aw==");

        assert_true(bool(r3));
        assert_equal(three_bytes, *r3);

        assert_true(bool(r2));
        assert_equal(two_bytes, *r2);

        assert_true(bool(r1));
        assert_equal(one_byte, *r1);
    }
};

} // namespace
