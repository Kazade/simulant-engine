#ifndef TEST_SHADER_H
#define TEST_SHADER_H

#include "kglt/kglt.h"
#include <kaztest/kaztest.h>

#include "global.h"

#include "kglt/gpu_program.h"

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
        kglt::GPUProgram::ptr s = kglt::GPUProgram::create();

        kglt::Mat4 ident;

        assert_false(s->is_complete());

        s->set_shader_source(kglt::SHADER_TYPE_VERTEX, "uniform vec3 c; attribute vec3 tns; void main(){ gl_Position = vec4(c, tns.x); }");
        s->set_shader_source(kglt::SHADER_TYPE_FRAGMENT, "void main(){ gl_FragColor = vec4(1.0); }");

        assert_false(s->is_compiled(kglt::SHADER_TYPE_VERTEX));
        assert_false(s->is_compiled(kglt::SHADER_TYPE_FRAGMENT));

        s->compile(kglt::SHADER_TYPE_VERTEX);

        assert_true(s->is_compiled(kglt::SHADER_TYPE_VERTEX));
        assert_false(s->is_compiled(kglt::SHADER_TYPE_FRAGMENT));

        s->build();

        assert_true(s->is_compiled(kglt::SHADER_TYPE_VERTEX));
        assert_true(s->is_compiled(kglt::SHADER_TYPE_FRAGMENT));
        assert_true(s->is_complete());

        s->activate();
        s->uniforms().set_vec3("c", kglt::Vec3());

        s->attributes().set_location("tns", 1);
        auto loc = s->attributes().locate("tns");

        assert_equal(1, loc);
    }

};

#endif // TEST_SHADER_H
