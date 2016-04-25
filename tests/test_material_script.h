#ifndef TEST_MATERIAL_SCRIPT_H
#define TEST_MATERIAL_SCRIPT_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"
#include "kglt/loaders/material_script.h"

class MaterialScriptTest : public KGLTTestCase {
public:
    void test_basic_material_script_parsing() {
        const std::string text = R"(
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
        )";

        auto mat = window->material(window->new_material());
        kglt::MaterialScript script((kglt::MaterialLanguageText(text))); //Most vexing parse \o/
        script.generate(*mat.__object);

        this->assert_equal((uint32_t)1, mat->pass_count());
        this->assert_equal((uint32_t)1, mat->pass(0)->texture_unit_count());

        //TODO: Add tests to make sure that the shader has compiled correctly
    }
};

#endif // TEST_MATERIAL_SCRIPT_H
