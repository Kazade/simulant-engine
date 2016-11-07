#pragma once

#include "simulant/simulant.h"
#include "kaztest/kaztest.h"

#include "global.h"

#ifndef SIMULANT_GL_VERSION_1X
#include "simulant/renderers/gl2x/gpu_program.h"
#endif

class ShaderTest : public SimulantTestCase {
public:
    void test_shader() {
#ifndef SIMULANT_GL_VERSION_1X
        smlt::GPUProgram::ptr prog = smlt::GPUProgram::create(
            "uniform vec3 c; attribute vec3 tns; void main(){ gl_Position = vec4(c, tns.x); }",
            "void main(){ gl_FragColor = vec4(1.0); }"
        );
        smlt::GPUProgramInstance::ptr s = smlt::GPUProgramInstance::create(prog);
        smlt::Mat4 ident;

        assert_false(s->program->is_compiled(smlt::SHADER_TYPE_VERTEX));
        assert_false(s->program->is_compiled(smlt::SHADER_TYPE_FRAGMENT));

        s->program->compile(smlt::SHADER_TYPE_VERTEX);

        assert_true(s->program->is_compiled(smlt::SHADER_TYPE_VERTEX));
        assert_false(s->program->is_compiled(smlt::SHADER_TYPE_FRAGMENT));

        s->program->build();

        assert_true(s->program->is_compiled(smlt::SHADER_TYPE_VERTEX));
        assert_true(s->program->is_compiled(smlt::SHADER_TYPE_FRAGMENT));
        assert_true(s->program->is_complete());

        s->program->activate();
        s->program->set_uniform_vec3("c", smlt::Vec3());

        s->program->set_attribute_location("tns", 1);
        auto loc = s->program->locate_attribute("tns");

        assert_equal(1, loc);
#endif
    }

};
