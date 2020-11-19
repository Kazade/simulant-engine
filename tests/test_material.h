#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

#include "simulant/assets/materials/material_property.inl.h"

class MaterialTest : public smlt::test::SimulantTestCase {
public:
    void test_material_initialization() {
        auto mat = window->shared_assets->material(window->shared_assets->new_material());

        mat->set_pass_count(1);

        this->assert_equal((uint32_t)1, mat->pass_count()); //Should return the default pass
        this->assert_true(smlt::Colour::WHITE == mat->pass(0)->diffuse()); //this->assert_true the default pass sets white as the default
        this->assert_true(smlt::Colour::WHITE == mat->pass(0)->ambient()); //this->assert_true the default pass sets white as the default
        this->assert_true(smlt::Colour::BLACK == mat->pass(0)->specular()); //this->assert_true the default pass sets black as the default
        this->assert_equal(0.0f, mat->pass(0)->shininess());
    }

    void test_material_variant() {
        /* Make sure that destructors are called correctly */

        smlt::MaterialVariant* variant = nullptr;

        smlt::TextureUnit unit(window->shared_assets->new_texture(8, 8));
        auto initial_count = unit.texture_.use_count();

        // Assign 1
        variant = new smlt::MaterialVariant();
        variant->set(unit);

        assert_equal(unit.texture_.use_count(), initial_count + 1);

        auto variant2 = *variant;

        assert_equal(unit.texture_.use_count(), initial_count + 2);

        variant->set(1);

        assert_equal(unit.texture_.use_count(), initial_count + 1);

        delete variant;

        assert_equal(unit.texture_.use_count(), initial_count + 1);
    }

    void test_material_applies_to_mesh() {
        smlt::MaterialID mid = window->shared_assets->new_material();
        smlt::MeshID mesh_id = window->shared_assets->new_mesh(smlt::VertexSpecification::POSITION_ONLY);
        auto mesh = window->shared_assets->mesh(mesh_id);
        smlt::SubMesh* sm = mesh->new_submesh_with_material("test", mid);
        this->assert_equal(mid, (smlt::MaterialID) sm->material());
    }

    void test_property_heirarchy() {
        auto mat = window->shared_assets->new_material();

        mat->set_diffuse(smlt::Colour::RED);
        mat->set_pass_count(2);

        auto pass1 = mat->pass(0);
        auto pass2 = mat->pass(1);

        assert_equal(pass1->diffuse(), smlt::Colour::RED);
        assert_equal(pass2->diffuse(), smlt::Colour::RED);

        pass1->set_diffuse(smlt::Colour::GREEN);

        assert_equal(pass1->diffuse(), smlt::Colour::GREEN);
        assert_equal(pass2->diffuse(), smlt::Colour::RED);
    }

    void test_pass_resizing() {
        auto mat1 = window->shared_assets->new_material();

        // Materials have a single pass by default, rightly or wrongly...
        assert_equal(mat1->pass_count(), 1);
        assert_equal(mat1->registered_material_object_count(), 2u);

        mat1->set_pass_count(2);
        assert_equal(mat1->pass_count(), 2);
        assert_equal(mat1->registered_material_object_count(), 3u);

        mat1->set_pass_count(1);
        assert_equal(mat1->pass_count(), 1);
        assert_equal(mat1->registered_material_object_count(), 2u);

        mat1->set_pass_count(2);
        assert_equal(mat1->pass_count(), 2);
        assert_equal(mat1->registered_material_object_count(), 3u);

        auto mat2 = window->shared_assets->clone_material(mat1);

        assert_equal(mat2->pass_count(), 2);
        assert_equal(mat2->registered_material_object_count(), 3u);
    }

    void test_material_copies() {
        auto mat1 = window->shared_assets->new_material();
        auto tex1 = window->shared_assets->new_texture(8, 8);

        mat1->set_diffuse(smlt::Colour::RED);
        mat1->set_diffuse_map(tex1);

        mat1->set_pass_count(2);
        mat1->pass(0)->set_diffuse(smlt::Colour::BLUE);

        auto mat2 = window->shared_assets->clone_material(mat1);

        assert_not_equal(mat1->id(), mat2->id());

        assert_equal(mat1->diffuse_map()->texture_id(), tex1->id());
        assert_equal(mat1->diffuse(), smlt::Colour::RED);
        assert_equal(mat1->pass_count(), 2);
        assert_equal(mat1->pass(0)->diffuse(), smlt::Colour::BLUE);
        assert_equal(mat1->pass(1)->diffuse(), smlt::Colour::RED);
        assert_equal(mat1->pass(0)->diffuse_map()->texture_id(), tex1->id());

        // Make sure the passes were copied
        assert_not_equal(mat1->pass(0), mat2->pass(0));

        assert_equal(mat2->diffuse_map()->texture_id(), tex1->id());
        assert_equal(mat2->diffuse(), smlt::Colour::RED);
        assert_equal(mat2->pass_count(), 2);
        assert_equal(mat2->pass(0)->diffuse(), smlt::Colour::BLUE);
        assert_equal(mat2->pass(1)->diffuse(), smlt::Colour::RED);
        assert_equal(mat2->pass(0)->diffuse_map()->texture_id(), tex1->id());

        mat2->set_diffuse(smlt::Colour::GREEN);
        assert_equal(mat2->pass(1)->diffuse(), smlt::Colour::GREEN);
    }

    void test_texture_unit() {
        auto mat = window->shared_assets->new_material();
        auto tex = window->shared_assets->new_texture(8, 8);

        mat->set_diffuse_map(tex);
        mat->set_pass_count(2);

        auto pass1 = mat->pass(0);
        auto pass2 = mat->pass(1);

        assert_equal(pass1->diffuse_map()->texture_id(), tex);
        assert_equal(pass2->diffuse_map()->texture_id(), tex);

        auto tex2 = window->shared_assets->new_texture(8, 8);

        pass1->set_diffuse_map(tex2);

        assert_equal(pass1->diffuse_map()->texture_id(), tex2);
        assert_equal(pass2->diffuse_map()->texture_id(), tex);

        /* Now to test scrolling */
        pass1->diffuse_map()->scroll_x(0.5f);
        assert_equal(pass1->diffuse_map()->texture_matrix()[12], 0.5f);
    }

    void test_shininess_is_clamped() {
        /* OpenGL requires that the specular exponent is between
         * 0 and 128, this checks that we clamp the value outside that
         * range */

        auto mat = window->shared_assets->new_material();
        mat->set_shininess(1000);
        assert_equal(mat->shininess(), 128);
        mat->set_shininess(-100);
        assert_equal(mat->shininess(), 0);
    }

    void test_pass_material_set_on_clone() {
        auto material = window->shared_assets->clone_default_material();

        assert_equal(material->pass(0)->material()->id(), material->id());
    }

    void test_setting_texture_unit_increases_refcount() {
        auto mat = window->shared_assets->new_material();
        mat->set_pass_count(1);

        auto texture = window->shared_assets->new_texture(8, 8);
        assert_equal(texture.use_count(), 2);

        mat->set_diffuse_map(texture);

        assert_equal(mat->diffuse_map()->texture_id(), texture->id());

        /* This takes some explanation. Basically the texture unit is copied
         * across all entries when set. There is 1 pass, plus 1 material so the
         * use count goes up by 2, not 1 */
        assert_equal(texture.use_count(), 4);
    }

    // FIXME: Restore this
    void test_reflectiveness() {
        /*
        smlt::MaterialID mid = window->shared_assets->new_material();
        auto mat = window->shared_assets->material(mid);
        mat->set_pass_count(1);

        auto pass = mat->pass(0);

        assert_false(pass->is_reflective());
        assert_false(mat->has_reflective_pass());
        assert_equal(0.0, pass->albedo());
        assert_equal(0, pass->reflection_texture_unit());

        pass->set_albedo(0.5);

        assert_equal(0.5, pass->albedo());
        assert_true(pass->is_reflective());
        assert_true(mat->has_reflective_pass());  */
    }

    void test_polygon_mode() {
        smlt::MaterialPtr mat = window->shared_assets->new_material();
        mat->set_polygon_mode(smlt::POLYGON_MODE_FILL);
        assert_equal(mat->polygon_mode(), smlt::POLYGON_MODE_FILL);

        mat->set_polygon_mode(smlt::POLYGON_MODE_LINE);
        assert_equal(mat->polygon_mode(), smlt::POLYGON_MODE_LINE);
    }
};
