#pragma once

#include <simulant/test.h>
#include "simulant/vfs.h"

namespace {

using namespace smlt;

class LocalisationTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        smlt::test::SimulantTestCase::set_up();
        smlt::get_app()->activate_language("en_US");
    }

    void test_activate_language_from_binary_data() {
        const std::string ARB_SAMPLE = R"({
    "@@locale": "fr",
    "start_text": "appuyez sur Start",
    "@start_text": {
        "type": "text",
        "context": "Title Screen",
        "source_text": "Press Start"
    }
}
)";
        std::vector<uint8_t> data(std::begin(ARB_SAMPLE), std::end(ARB_SAMPLE));
        assert_true(get_app()->activate_language_from_arb_data(&data[0], data.size()));
        assert_equal(get_app()->active_language(), "fr");
    }

    void test_basic_usage() {
        const char* ARB_SAMPLE = R"({
    "@@locale": "fr",
    "start_text": "appuyez sur Start",
    "@start_text": {
        "type": "text",
        "context": "Title Screen",
        "source_text": "Press Start"
    }
}
)";

        /* Can't activate French as we have no valid ARB file on the path */
        assert_false(get_app()->activate_language("fr"));

        /* Just returns the source text */
        assert_equal(get_app()->translated_text("Press Start"), "Press Start");

        auto locale_dir = kfs::path::join(kfs::temp_dir(), "locales");
        kfs::make_dirs(locale_dir);

        auto file = kfs::path::join(locale_dir, "fr.arb");
        get_app()->vfs->add_search_path(locale_dir);

        std::ofstream arb(file);
        arb.write(ARB_SAMPLE, strlen(ARB_SAMPLE));
        arb.close();

        assert_true(get_app()->activate_language("fr"));
        assert_equal(get_app()->translated_text("Press Start"), "appuyez sur Start");
        assert_equal(get_app()->active_language(), "fr");
    }

    void test_loading_unicode() {
        const char* ARB_SAMPLE = u8R"({
    "@@locale": "fr",
    "start_text": "RÉSEAU",
    "@start_text": {
        "type": "text",
        "context": "Menu",
        "source_text": "NETWORK"
    }
}
)";
        auto locale_dir = kfs::path::join(kfs::temp_dir(), "locales");
        kfs::make_dirs(locale_dir);

        auto file = kfs::path::join(locale_dir, "fr.arb");

        std::ofstream arb(file);
        arb.write(ARB_SAMPLE, strlen(ARB_SAMPLE));
        arb.close();

        smlt::get_app()->vfs->add_search_path(locale_dir);

        assert_true(get_app()->activate_language("fr"));
        assert_equal(get_app()->translated_text("NETWORK").encode(), u8"RÉSEAU");
    }
};

}
