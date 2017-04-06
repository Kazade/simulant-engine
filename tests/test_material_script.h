#ifndef TEST_MATERIAL_SCRIPT_H
#define TEST_MATERIAL_SCRIPT_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"
#include "global.h"
#include "simulant/loaders/material_script.h"

class MaterialScriptTest : public SimulantTestCase {
public:
    void test_basic_material_script_parsing() {
        auto root = kfs::path::dir_name(kfs::path::dir_name(__FILE__));

        window->resource_locator->add_search_path(
            kfs::path::join(root, "samples/data")
        );

        const std::string text = R"(
                BEGIN(pass)
                    SET(TEXTURE_UNIT 0 "sample.tga")

                    BEGIN_DATA(vertex)
                        #version 120
                        void main() {
                            gl_Position = vec4(1.0);
                        }
                    END_DATA(vertex)
                    BEGIN_DATA(fragment)
                        #version 120
                        void main() {
                            gl_FragColor = vec4(1.0);
                        }
                    END_DATA(fragment)
                END(pass)
        )";

        auto mat = window->shared_assets->material(window->shared_assets->new_material());
        smlt::MaterialScript script((smlt::MaterialLanguageText(text))); //Most vexing parse \o/
        script.generate(*mat);

        this->assert_equal((uint32_t)1, mat->pass_count());
        this->assert_equal((uint32_t)1, mat->pass(0)->texture_unit_count());

        //TODO: Add tests to make sure that the shader has compiled correctly
    }
};

#endif // TEST_MATERIAL_SCRIPT_H
