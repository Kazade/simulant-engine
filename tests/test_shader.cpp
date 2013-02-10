

#include <UnitTest++.h>

#include <vector>

#include "kglt/shortcuts.h"
#include "kglt/kglt.h"
#include "kglt/object.h"

using namespace kglt;

TEST(test_shader_params) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    ShaderID sid = scene.new_shader();
    CHECK(sid);

    ShaderProgram& s = scene.shader(sid);
    s.activate();

    kmMat4 ident;
    kmMat4Identity(&ident);

    //Register the auto uniforms that the shader requires
    s.params().register_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX, "modelview_projection");
    s.params().register_auto(SP_AUTO_MATERIAL_DIFFUSE, "diffuse_colour");

    //Register the vertex attributes the shader needs
    s.params().register_attribute(SP_ATTR_VERTEX_POSITION, "vertex_position");
    s.params().register_attribute(SP_ATTR_VERTEX_NORMAL, "vertex_normal");

    //Finally set some uniform constants manually
    s.params().set_int("texture_1", 0);
    s.params().set_mat4x4("matrix", ident);

    CHECK(s.params().uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX));
    CHECK(s.params().uses_auto(SP_AUTO_MATERIAL_DIFFUSE));
    CHECK(!s.params().uses_auto(SP_AUTO_MATERIAL_SPECULAR));

    CHECK(s.params().uses_attribute(SP_ATTR_VERTEX_NORMAL));
    CHECK(s.params().uses_attribute(SP_ATTR_VERTEX_POSITION));
    CHECK(!s.params().uses_attribute(SP_ATTR_VERTEX_DIFFUSE));
}



