#ifndef TEST_MATERIAL_SCRIPT_H
#define TEST_MATERIAL_SCRIPT_H

#include <sstream>

#include "simulant/simulant.h"
#include "simulant/test.h"

#include "simulant/loaders/material_script.h"

class MaterialScriptTest : public smlt::test::SimulantTestCase {
public:
    void test_property_definitions_are_loaded_correctly() {
        const std::string text = R"(
{
    "custom_properties": [
        {
            "name": "texture_map",
            "type": "texture",
            "default": null
        },
        {
            "name": "enable_texturing",
            "type": "bool",
            "default": true
        }
    ],

    "property_values": {
        "texture_map": "assets/textures/simulant-icon.png",
        "s_material_diffuse": "1 0 1 0",
        "s_material_ambient": "1 1 1 1"
    },

    "passes": [
        {
            "iteration": "once"
        }
    ]
}
        )";

        auto mat = application->shared_assets->create_material();

        auto stream = std::make_shared<std::stringstream>();
        (*stream) << text;

        smlt::MaterialScript script(stream, "some/path");
        script.init();
        script.generate(*mat);

        assert_equal(mat->pass_count(), 1);

//        auto prop_id = mat->find_property_id("texture_map");
//        assert_true(mat->property(prop_id)->is_custom);
//        assert_equal(mat->property(prop_id)->type, smlt::MATERIAL_PROPERTY_TYPE_TEXTURE);
        assert_equal(mat->pass(0)->iteration_type(), smlt::ITERATION_TYPE_ONCE);
        assert_equal(mat->diffuse(), smlt::Color(1, 0, 1, 0));
        assert_equal(mat->ambient(), smlt::Color(1, 1, 1, 1));

        script.clean_up();
    }
};

#endif // TEST_MATERIAL_SCRIPT_H
