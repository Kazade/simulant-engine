#pragma once

#include "kglt/kglt.h"
#include "kaztest/kaztest.h"

#include "global.h"

#include "kglt/gpu_program.h"

class ShaderTest : public KGLTTestCase {
public:
    void test_shader() {
        kglt::GPUProgram::ptr prog = kglt::GPUProgram::create(
            "uniform vec3 c; attribute vec3 tns; void main(){ gl_Position = vec4(c, tns.x); }",
            "void main(){ gl_FragColor = vec4(1.0); }"
        );
        kglt::GPUProgramInstance::ptr s = kglt::GPUProgramInstance::create(prog);
        kglt::Mat4 ident;

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

        s->program->set_attribute_location("tns", 1);
        auto loc = s->program->locate_attribute("tns");

        assert_equal(1, loc);
    }

};
