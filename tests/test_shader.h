#ifndef TEST_SHADER_H
#define TEST_SHADER_H

#include "kglt/kglt.h"
#include "kglt/kazbase/testing.h"

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

    void test_shader_params() {
        kglt::Scene& scene = window->scene();

        kglt::ShaderID sid = scene.new_shader();
        assert_true(sid);

        auto material = scene.material(scene.default_material_id());
        kglt::ShaderRef tmp = scene.shader(material->technique().pass(0).shader_id());
        kglt::ShaderProgram& s = *tmp.lock();

        kmMat4 ident;
        kmMat4Identity(&ident);

        //Register the auto uniforms that the shader requires
        s.params().register_auto(kglt::SP_AUTO_MODELVIEW_PROJECTION_MATRIX, "modelview_projection");
        s.params().register_auto(kglt::SP_AUTO_MATERIAL_DIFFUSE, "diffuse_colour");

        //Register the vertex attributes the shader needs
        s.params().register_attribute(kglt::SP_ATTR_VERTEX_POSITION, "vertex_position");
        s.params().register_attribute(kglt::SP_ATTR_VERTEX_NORMAL, "vertex_normal");

        //Finally set some uniform constants manually
        s.params().set_int("texture_1", 0);
        s.params().set_mat4x4("matrix", ident);

        assert_true(s.params().uses_auto(kglt::SP_AUTO_MODELVIEW_PROJECTION_MATRIX));
        assert_true(s.params().uses_auto(kglt::SP_AUTO_MATERIAL_DIFFUSE));
        assert_false(s.params().uses_auto(kglt::SP_AUTO_MATERIAL_SPECULAR));

        assert_true(s.params().uses_attribute(kglt::SP_ATTR_VERTEX_NORMAL));
        assert_true(s.params().uses_attribute(kglt::SP_ATTR_VERTEX_POSITION));
        assert_false(s.params().uses_attribute(kglt::SP_ATTR_VERTEX_DIFFUSE));
    }

};

#endif // TEST_SHADER_H
