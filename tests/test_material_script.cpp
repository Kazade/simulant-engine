#include <unittest++/UnitTest++.h>

#include <string>
#include "kglt/kglt.h"
#include "kglt/material_script.h"

using namespace kglt;

TEST(test_basic_material_script_parsing) {
    const std::string text = R"(
        BEGIN(technique)
            SET(NAME "my_technique")
            BEGIN(pass)
                SET(TEXTURE_UNIT0 "sample_data/sample.png")

                BEGIN(vertex)
                    #version 120
                    void main() {
                        gl_Position = vec4(1.0);
                    }
                END(vertex)
                BEGIN(fragment)
                    #version 120
                    void main() {
                        gl_FragColor = vec4(1.0);
                    }
                END(fragment)
            END(pass)
        END(technique)
    )";

    kglt::Window::ptr window = kglt::Window::create();

    MaterialScript script(window->scene(), MaterialLanguageText(text));
    MaterialID material_id = script.generate();

    Material& mat = window->scene().material(material_id);

    CHECK_EQUAL(1, mat.technique_count());
    CHECK_EQUAL(1, mat.technique("my_technique").pass_count());
    CHECK_EQUAL(1, mat.technique("my_technique").pass(0).texture_unit_count());
}
