#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


#ifndef _arch_dreamcast
#include "simulant/renderers/gl2x/gpu_program.h"
#endif

class ShaderTest : public smlt::test::SimulantTestCase {
public:
    void test_shader() {
#ifndef _arch_dreamcast
        smlt::GPUProgram::ptr program = smlt::GPUProgram::create(
            smlt::GPUProgramID(1),
            window->renderer,
            "uniform vec3 c; attribute vec3 tns; void main(){ gl_Position = vec4(c, tns.x); }",
            "void main(){ gl_FragColor = vec4(1.0); }"
        );
        smlt::Mat4 ident;

        assert_false(program->is_compiled(smlt::SHADER_TYPE_VERTEX));
        assert_false(program->is_compiled(smlt::SHADER_TYPE_FRAGMENT));

        program->compile(smlt::SHADER_TYPE_VERTEX);

        assert_true(program->is_compiled(smlt::SHADER_TYPE_VERTEX));
        assert_false(program->is_compiled(smlt::SHADER_TYPE_FRAGMENT));

        program->build();

        assert_true(program->is_compiled(smlt::SHADER_TYPE_VERTEX));
        assert_true(program->is_compiled(smlt::SHADER_TYPE_FRAGMENT));
        assert_true(program->is_complete());

        program->activate();
        program->set_uniform_vec3("c", smlt::Vec3());

        program->set_attribute_location("tns", 1);
        auto loc = program->locate_attribute("tns");

        assert_equal(1, loc);
#endif
    }

};
