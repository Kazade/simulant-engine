#ifndef TEST_MATERIAL_SCRIPT_H
#define TEST_MATERIAL_SCRIPT_H

#include "kglt/kazbase/testing.h"
#include "kglt/kglt.h"
#include "global.h"
#include "kglt/loaders/material_script.h"

class MaterialScriptTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_basic_material_script_parsing() {
        const std::string text = R"(
            BEGIN(technique "my_technique")
                BEGIN(pass)
                    SET(TEXTURE_UNIT "../sample_data/sample.tga")

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
            END(technique)
        )";

        auto mat = window->scene().material(window->scene().new_material());
        kglt::MaterialScript script((MaterialLanguageText(text))); //Most vexing parse \o/
        script.generate(*mat);

        this->assert_equal((uint32_t)2, mat->technique_count());
        this->assert_true(mat->has_technique(DEFAULT_MATERIAL_SCHEME));
        this->assert_true(mat->has_technique("my_technique"));
        this->assert_equal((uint32_t)1, mat->technique("my_technique").pass_count());
        this->assert_equal((uint32_t)1, mat->technique("my_technique").pass(0).texture_unit_count());

        //TODO: Add tests to make sure that the shader has compiled correctly
    }
};

#endif // TEST_MATERIAL_SCRIPT_H
