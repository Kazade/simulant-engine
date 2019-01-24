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
        },
    ],

    "property_values": {
        "texture_map": "assets/textures/mytexture.png",
        "s_diffuse": "1 0 0 0",
        "s_ambient": "1 1 1 1"
    }

    "passes" [
        {
            "iteration": "once",
            "vertex_shader": "assets/shader/default.vert",
            "fragment_shader": "assets/shader/default.frag"
        }
    ]
}
        )";

        auto mat = window->shared_assets->material(window->shared_assets->new_material());

        auto stream = std::make_shared<std::stringstream>();
        (*stream) << text;

        smlt::MaterialScript script(stream);
        script.generate(*mat);

        assert_equal(mat->pass_count(), 1);
        assert_true(mat->property("texture_map")->is_custom());
        assert_equal(mat->property("texture_map")->type(), smlt::MATERIAL_PROPERTY_TYPE_TEXTURE);
        assert_true(mat->property("texture_map")->is_set());
        assert_equal(mat->property("texture_map")->value<TextureUnit>().filename, "assets/textures/mytexture.png");
/*
        assert_equal(mat->pass(0)->iteration, smlt::MATERIAL_PASS_ITERATION_ONCE);
        assert_equal(mat->pass(0)->vertex_shader->filename(), "assets/shader/default.vert");
        assert_true(mat->pass(0)->vertex_shader->shader_id());

        mat->each_pass([&](uint32_t i, MaterialPass* pass) {
            assert_equal(pass->iteration, smlt::MATERIAL_PASS_ITERATION_ONCE);
            assert_equal(pass->vertex_shader.filename(), "assets/shader/default.vert");
            assert_true(pass->vertex_shader.shader_id());
        }); */

        //TODO: Add tests to make sure that the shader has compiled correctly
    }
};

#endif // TEST_MATERIAL_SCRIPT_H
