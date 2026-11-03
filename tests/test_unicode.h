#pragma once

#include <string>
#include <utility>
#include <vector>

#include "simulant/test.h"
#include "simulant/utils/unicode.h"

namespace {

using namespace smlt;

class UnicodeEncodeTests : public test::TestCase {
public:
    /* Encodes, failing the test rather than asserting in value() if the
     * encoding was rejected */
    std::string encoded(const unicode& text, const char* encoding) {
        auto result = text.encode(encoding);
        assert_true(result.has_value());
        return result.value();
    }

    /* Note: string literals below are split after a hex escape where the next
     * character could be read as part of it (e.g. "\xACb") */

    void test_default_encoding_is_utf8() {
        const unicode text(u"RÉSEAU €");
        assert_equal(encoded(text, "utf-8"), text.encode().value());
    }

    void test_empty_string_is_valid_in_every_encoding() {
        const unicode text(u"");
        for(auto encoding: {"utf-8", "utf8", "latin-1", "latin1", "iso-8859-1", "ascii"}) {
            assert_equal(encoded(text, encoding), std::string());
        }
    }

    void test_unknown_encodings_return_no_value() {
        const unicode text(u"hello");
        for(auto encoding: {"utf-16", "latin-2", "bogus", ""}) {
            assert_false(text.encode(encoding).has_value());
        }
    }

    void test_ascii_text_is_the_same_in_every_encoding() {
        const unicode text(u"Hello, World! 123");
        for(auto encoding: {"utf-8", "utf8", "latin-1", "latin1", "iso-8859-1", "ascii"}) {
            assert_equal(encoded(text, encoding), std::string("Hello, World! 123"));
        }
    }

    void test_utf8_aliases_match() {
        const unicode text(u"RÉSEAU € \U0001F600");
        assert_equal(encoded(text, "utf8"), encoded(text, "utf-8"));
    }

    void test_utf8_multibyte_sequences() {
        assert_equal(encoded(unicode(u"é"), "utf-8"), std::string("\xC3\xA9"));
        assert_equal(encoded(unicode(u"€"), "utf-8"), std::string("\xE2\x82\xAC"));

        /* A surrogate pair is one character, in four bytes */
        assert_equal(
            encoded(unicode(u"a\U0001F600z"), "utf-8"),
            std::string("a\xF0\x9F\x98\x80" "z")
        );
    }

    void test_utf8_replaces_unpaired_surrogates() {
        const ustring high_then_char = {u'a', char16_t(0xD83D), u'z'};
        assert_equal(
            encoded(unicode(high_then_char.begin(), high_then_char.end()), "utf-8"),
            std::string("a?z")
        );

        const ustring high_at_end = {u'a', char16_t(0xD83D)};
        assert_equal(
            encoded(unicode(high_at_end.begin(), high_at_end.end()), "utf-8"),
            std::string("a?")
        );

        const ustring low_alone = {u'a', char16_t(0xDE00), u'z'};
        assert_equal(
            encoded(unicode(low_alone.begin(), low_alone.end()), "utf-8"),
            std::string("a?z")
        );

        /* The wrong way around isn't a pair */
        const ustring reversed = {char16_t(0xDE00), char16_t(0xD83D)};
        assert_equal(
            encoded(unicode(reversed.begin(), reversed.end()), "utf-8"),
            std::string("??")
        );

        /* A valid pair right after an unpaired surrogate is still encoded */
        const ustring then_pair = {char16_t(0xD83D), char16_t(0xD83D), char16_t(0xDE00)};
        assert_equal(
            encoded(unicode(then_pair.begin(), then_pair.end()), "utf-8"),
            std::string("?\xF0\x9F\x98\x80")
        );
    }

    void test_utf8_keeps_embedded_nulls() {
        const ustring text = {u'a', u'\0', u'z'};
        assert_equal(
            encoded(unicode(text.begin(), text.end()), "utf-8"),
            std::string("a\0z", 3)
        );
    }

    void test_utf8_code_point_boundaries() {
        /* Where the encoded length changes, and either side of the surrogates */
        const std::vector<std::pair<ustring, std::string>> cases = {
            {ustring{char16_t(0x007F)}, "\x7F"},
            {ustring{char16_t(0x0080)}, "\xC2\x80"},
            {ustring{char16_t(0x07FF)}, "\xDF\xBF"},
            {ustring{char16_t(0x0800)}, "\xE0\xA0\x80"},
            {ustring{char16_t(0xD7FF)}, "\xED\x9F\xBF"},
            {ustring{char16_t(0xE000)}, "\xEE\x80\x80"},
            {ustring{char16_t(0xFFFF)}, "\xEF\xBF\xBF"},
            {ustring{char16_t(0xD800), char16_t(0xDC00)}, "\xF0\x90\x80\x80"},
            {ustring{char16_t(0xDBFF), char16_t(0xDFFF)}, "\xF4\x8F\xBF\xBF"},
        };

        for(auto& c: cases) {
            assert_equal(encoded(unicode(c.first.begin(), c.first.end()), "utf-8"), c.second);
        }
    }

    void test_utf8_round_trip() {
        const std::string text("R\xC3\x89SEAU \xE2\x82\xAC \xF0\x9F\x98\x80");
        assert_equal(encoded(unicode(text, "utf-8"), "utf-8"), text);
    }

    void test_latin1_aliases_match() {
        const unicode text(u"RÉSEAU € \U0001F600");
        assert_equal(encoded(text, "latin1"), encoded(text, "latin-1"));
        assert_equal(encoded(text, "iso-8859-1"), encoded(text, "latin-1"));
    }

    void test_latin1_is_one_byte_per_character() {
        assert_equal(encoded(unicode(u"RÉSEAU"), "latin-1"), std::string("R\xC9" "SEAU"));
    }

    void test_latin1_range_boundary() {
        /* The last character that fits, and the first that doesn't */
        assert_equal(encoded(unicode(u"ÿ"), "latin-1"), std::string("\xFF"));
        assert_equal(encoded(unicode(u"Ā"), "latin-1"), std::string("?"));
    }

    void test_latin1_replaces_unrepresentable_characters() {
        assert_equal(encoded(unicode(u"a€z"), "latin-1"), std::string("a?z"));
    }

    void test_latin1_surrogate_pair_is_one_replacement() {
        assert_equal(encoded(unicode(u"a\U0001F600z"), "latin-1"), std::string("a?z"));
        assert_equal(
            encoded(unicode(u"\U0001F600\U0001F601"), "latin-1"), std::string("??")
        );
    }

    void test_latin1_unpaired_surrogates_are_one_replacement_each() {
        const ustring high_then_char = {u'a', char16_t(0xD83D), u'z'};
        assert_equal(
            encoded(unicode(high_then_char.begin(), high_then_char.end()), "latin-1"),
            std::string("a?z")
        );

        const ustring high_at_end = {u'a', char16_t(0xD83D)};
        assert_equal(
            encoded(unicode(high_at_end.begin(), high_at_end.end()), "latin-1"),
            std::string("a?")
        );

        const ustring low_alone = {u'a', char16_t(0xDE00), u'z'};
        assert_equal(
            encoded(unicode(low_alone.begin(), low_alone.end()), "latin-1"),
            std::string("a?z")
        );

        /* The wrong way around isn't a pair */
        const ustring reversed = {char16_t(0xDE00), char16_t(0xD83D)};
        assert_equal(
            encoded(unicode(reversed.begin(), reversed.end()), "latin-1"),
            std::string("??")
        );
    }

    void test_latin1_keeps_embedded_nulls() {
        const ustring text = {u'a', u'\0', u'z'};
        assert_equal(
            encoded(unicode(text.begin(), text.end()), "latin-1"),
            std::string("a\0z", 3)
        );
    }

    void test_latin1_round_trip() {
        /* Every byte, except for 0 as the constructor takes a C string */
        std::string bytes;
        for(int i = 1; i < 256; ++i) {
            bytes.push_back((char) i);
        }

        for(auto encoding: {"latin-1", "latin1", "iso-8859-1"}) {
            assert_equal(encoded(unicode(bytes, encoding), encoding), bytes);
        }
    }

    void test_ascii_range_boundary() {
        assert_equal(encoded(unicode(u"\u007F"), "ascii"), std::string("\x7F"));
        assert_equal(encoded(unicode(u"\u0080"), "ascii"), std::string("?"));
    }

    void test_ascii_replaces_unrepresentable_characters() {
        assert_equal(encoded(unicode(u"RÉSEAU €"), "ascii"), std::string("R?SEAU ?"));
    }

    void test_ascii_surrogate_pair_is_one_replacement() {
        const ustring one = {u'a', char16_t(0xD83D), char16_t(0xDE00), u'z'};
        assert_equal(encoded(unicode(one.begin(), one.end()), "ascii"), std::string("a?z"));

        const ustring two = {char16_t(0xD83D), char16_t(0xDE00), char16_t(0xD83D), char16_t(0xDE01)};
        assert_equal(encoded(unicode(two.begin(), two.end()), "ascii"), std::string("??"));
    }

    void test_ascii_unpaired_surrogates_are_one_replacement_each() {
        const ustring high_at_end = {u'a', char16_t(0xD83D)};
        assert_equal(
            encoded(unicode(high_at_end.begin(), high_at_end.end()), "ascii"),
            std::string("a?")
        );

        const ustring reversed = {char16_t(0xDE00), char16_t(0xD83D)};
        assert_equal(
            encoded(unicode(reversed.begin(), reversed.end()), "ascii"),
            std::string("??")
        );
    }
};

}
