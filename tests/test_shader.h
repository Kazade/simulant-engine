#ifndef TEST_SHADER_H
#define TEST_SHADER_H

#include "kglt/kglt.h"
#include <kaztest/kaztest.h>

#include "global.h"

#include "kglt/gpu_program.h"

class ShaderTest : public KGLTTestCase {
public:
    void test_shader() {
        kglt::GPUProgram::ptr prog = kglt::GPUProgram::create();
        kglt::GPUProgramInstance::ptr s = kglt::GPUProgramInstance::create(prog);

        kglt::Mat4 ident;

        assert_false(s->program->is_complete());

        s->program->set_shader_source(kglt::SHADER_TYPE_VERTEX, "uniform vec3 c; attribute vec3 tns; void main(){ gl_Position = vec4(c, tns.x); }");
        s->program->set_shader_source(kglt::SHADER_TYPE_FRAGMENT, "void main(){ gl_FragColor = vec4(1.0); }");

        assert_false(s->program->is_compiled(kglt::SHADER_TYPE_VERTEX));
        assert_false(s->program->is_compiled(kglt::SHADER_TYPE_FRAGMENT));

        s->program->compile(kglt::SHADER_TYPE_VERTEX);

        assert_true(s->program->is_compiled(kglt::SHADER_TYPE_VERTEX));
        assert_false(s->program->is_compiled(kglt::SHADER_TYPE_FRAGMENT));

        s->program->build();

        assert_true(s->program->is_compiled(kglt::SHADER_TYPE_VERTEX));
        assert_true(s->program->is_compiled(kglt::SHADER_TYPE_FRAGMENT));
        assert_true(s->program->is_complete());

        s->program->activate();
        s->program->set_uniform_vec3("c", kglt::Vec3());

        s->attributes->set_location("tns", 1);
        auto loc = s->attributes->locate("tns");

        assert_equal(1, loc);
    }

};

#endif // TEST_SHADER_H
