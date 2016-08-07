#ifndef TEST_MATERIAL_H
#define TEST_MATERIAL_H

#include "kaztest/kaztest.h"

#include "kglt/kglt.h"
#include "global.h"

class MaterialTest : public KGLTTestCase {
public:
    void test_material_initialization() {
        auto mat = window->shared_assets->material(window->shared_assets->new_material());

        mat->new_pass(); //Create a pass
        this->assert_equal((uint32_t)1, mat->pass_count()); //Should return the default pass
        this->assert_true(kglt::Colour::WHITE == mat->pass(0)->diffuse()); //this->assert_true the default pass sets white as the default
        this->assert_true(kglt::Colour::WHITE == mat->pass(0)->ambient()); //this->assert_true the default pass sets white as the default
        this->assert_true(kglt::Colour::WHITE == mat->pass(0)->specular()); //this->assert_true the default pass sets white as the default
        this->assert_equal(0.0, mat->pass(0)->shininess());
    }

    void test_material_applies_to_mesh() {
        kglt::MaterialID mid = window->shared_assets->new_material();
        kglt::MeshID mesh_id = window->shared_assets->new_mesh(kglt::VertexSpecification::POSITION_ONLY);
        auto mesh = window->shared_assets->mesh(mesh_id);
        kglt::SubMeshID idx = mesh->new_submesh_with_material(mid);
        this->assert_equal(mid, mesh->submesh(idx)->material_id());
    }

    void test_reflectiveness() {
        kglt::MaterialID mid = window->shared_assets->new_material();
        auto mat = window->shared_assets->material(mid);
        uint32_t pass_id = mat->new_pass();
        auto pass = mat->pass(pass_id);

        assert_false(pass->is_reflective());
        assert_false(mat->has_reflective_pass());
        assert_equal(0.0, pass->albedo());
        assert_equal(0, pass->reflection_texture_unit());

        pass->set_albedo(0.5);

        assert_equal(0.5, pass->albedo());
        assert_true(pass->is_reflective());
        assert_true(mat->has_reflective_pass());
    }
};

#endif // TEST_MATERIAL_H
