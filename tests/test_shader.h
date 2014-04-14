#ifndef TEST_SHADER_H
#define TEST_SHADER_H

#include "kglt/kglt.h"
#include <kaztest/kaztest.h>

#include "global.h"

class ShaderTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_shader() {
        GPUProgram s;

        kglt::Mat4 ident;

        assert_false(s.is_complete());

        s.set_shader_source(SHADER_TYPE_VERTEX, "void main(){}");
        s.set_shader_source(SHADER_TYPE_FRAGMENT, "void main(){}");

        assert_false(s.is_compiled(SHADER_TYPE_VERTEX));
        assert_false(s.is_compiled(SHADER_TYPE_FRAGMENT));

        s.compile(SHADER_TYPE_VERTEX);

        assert_true(s.is_compiled(SHADER_TYPE_VERTEX));
        assert_false(s.is_compiled(SHADER_TYPE_FRAGMENT));

        s.build();

        assert_true(s.is_compiled(SHADER_TYPE_VERTEX));
        assert_true(s.is_compiled(SHADER_TYPE_FRAGMENT));
        assert_true(s.is_complete());

        s.uniforms().set_int("texture_1", 0);
        s.uniforms().set_mat4x4("matrix", ident);

        s.attributes().set_location("modelview_projection", 1);
        auto loc = s.attributes().get_location("modelview_projection");

        assert_equal(1, loc);
    }

};

#endif // TEST_SHADER_H
