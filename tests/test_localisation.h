#pragma once

#include <simulant/test.h>
#include "simulant/vfs.h"

namespace {

using namespace smlt;

class LocalisationTests : public smlt::test::SimulantTestCase {
public:
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
};

}
