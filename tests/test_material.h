#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

#include "simulant/assets/materials/core/core_material.h"
#include "simulant/assets/materials/core/material_property_overrider.h"

namespace {

using namespace smlt;

class MaterialTest : public smlt::test::SimulantTestCase {
public:
    void test_material_initialization() {
        auto mat = application->shared_assets->material(application->shared_assets->new_material());

        mat->set_pass_count(1);

        this->assert_equal((uint32_t)1, mat->pass_count()); //Should return the default pass
        this->assert_true(smlt::Colour::WHITE == mat->pass(0)->diffuse()); //this->assert_true the default pass sets white as the default
        this->assert_true(smlt::Colour::WHITE == mat->pass(0)->ambient()); //this->assert_true the default pass sets white as the default
        this->assert_true(smlt::Colour::BLACK == mat->pass(0)->specular()); //this->assert_true the default pass sets black as the default
        this->assert_equal(0.0f, mat->pass(0)->shininess());
    }

    void test_material_applies_to_mesh() {
        smlt::MaterialID mid = application->shared_assets->new_material();
        smlt::MeshID mesh_id = application->shared_assets->new_mesh(smlt::VertexSpecification::POSITION_ONLY);
        auto mesh = application->shared_assets->mesh(mesh_id);
        smlt::SubMesh* sm = mesh->new_submesh_with_material("test", mid);
        this->assert_equal(mid, (smlt::MaterialID) sm->material());
    }

    void test_property_heirarchy() {
        auto mat = application->shared_assets->new_material();

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
        auto mat1 = application->shared_assets->new_material();

        // Materials have a single pass by default, rightly or wrongly...
        assert_equal(mat1->pass_count(), 1);

        mat1->set_pass_count(2);
        assert_equal(mat1->pass_count(), 2);

        mat1->set_pass_count(1);
        assert_equal(mat1->pass_count(), 1);

        mat1->set_pass_count(2);
        assert_equal(mat1->pass_count(), 2);

        auto mat2 = application->shared_assets->clone_material(mat1);

        assert_equal(mat2->pass_count(), 2);
    }

    void test_material_copies() {
        auto mat1 = application->shared_assets->new_material();
        auto tex1 = application->shared_assets->new_texture(8, 8);

        mat1->set_diffuse(smlt::Colour::RED);
        mat1->set_diffuse_map(tex1);

        mat1->set_pass_count(2);
        mat1->pass(0)->set_diffuse(smlt::Colour::BLUE);

        auto mat2 = application->shared_assets->clone_material(mat1);

        assert_not_equal(mat1->id(), mat2->id());

        assert_equal(mat1->diffuse_map(), tex1);
        assert_equal(mat1->diffuse(), smlt::Colour::RED);
        assert_equal(mat1->pass_count(), 2);
        assert_equal(mat1->pass(0)->diffuse(), smlt::Colour::BLUE);
        assert_equal(mat1->pass(1)->diffuse(), smlt::Colour::RED);
        assert_equal(mat1->pass(0)->diffuse_map(), tex1);

        // Make sure the passes were copied
        assert_not_equal(mat1->pass(0), mat2->pass(0));

        assert_equal(mat2->diffuse_map(), tex1);
        assert_equal(mat2->diffuse(), smlt::Colour::RED);
        assert_equal(mat2->pass_count(), 2);
        assert_equal(mat2->pass(0)->diffuse(), smlt::Colour::BLUE);
        assert_equal(mat2->pass(1)->diffuse(), smlt::Colour::RED);
        assert_equal(mat2->pass(0)->diffuse_map(), tex1);

        mat2->set_diffuse(smlt::Colour::GREEN);
        assert_equal(mat2->pass(1)->diffuse(), smlt::Colour::GREEN);
    }

    void test_texture_unit() {
        auto mat = application->shared_assets->new_material();
        auto tex = application->shared_assets->new_texture(8, 8);

        mat->set_diffuse_map(tex);
        mat->set_pass_count(2);

        auto pass1 = mat->pass(0);
        auto pass2 = mat->pass(1);

        assert_equal(pass1->diffuse_map(), tex);
        assert_equal(pass2->diffuse_map(), tex);

        auto tex2 = application->shared_assets->new_texture(8, 8);

        pass1->set_diffuse_map(tex2);

        assert_equal(pass1->diffuse_map(), tex2);
        assert_equal(pass2->diffuse_map(), tex);

        /* Now to test scrolling */
        auto dm = pass1->diffuse_map_matrix();
        dm[12] = 0.5f;
        pass1->set_diffuse_map_matrix(dm);
        assert_equal(pass1->diffuse_map_matrix()[12], 0.5f);
    }

    void test_textures_enabled() {
        auto mat = application->shared_assets->new_material();

        // By default, all units are enabled
        assert_equal(
            mat->textures_enabled(),
            DIFFUSE_MAP_ENABLED | LIGHT_MAP_ENABLED | SPECULAR_MAP_ENABLED | NORMAL_MAP_ENABLED
        );

        mat->set_textures_enabled(DIFFUSE_MAP_ENABLED | LIGHT_MAP_ENABLED);

        assert_equal(mat->textures_enabled(), DIFFUSE_MAP_ENABLED | LIGHT_MAP_ENABLED);
    }

    void test_shininess_is_clamped() {
        /* OpenGL requires that the specular exponent is between
         * 0 and 128, this checks that we clamp the value outside that
         * range */

        auto mat = application->shared_assets->new_material();
        mat->set_shininess(1000);
        assert_equal(mat->shininess(), 128);
        mat->set_shininess(-100);
        assert_equal(mat->shininess(), 0);
    }

    void test_pass_material_set_on_clone() {
        auto material = application->shared_assets->clone_default_material();

        assert_equal(material->pass(0)->material()->id(), material->id());
    }

    void test_setting_texture_unit_increases_refcount() {
        auto mat = application->shared_assets->new_material();
        mat->set_pass_count(1);

        auto texture = application->shared_assets->new_texture(8, 8);
        assert_equal(texture.use_count(), 2);

        mat->set_diffuse_map(texture);

        assert_equal(mat->diffuse_map(), texture);

        assert_equal(texture.use_count(), 3);
    }

    // FIXME: Restore this
    void test_reflectiveness() {
        /*
        smlt::MaterialID mid = application->shared_assets->new_material();
        auto mat = application->shared_assets->material(mid);
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
        smlt::MaterialPtr mat = application->shared_assets->new_material();
        mat->set_polygon_mode(smlt::POLYGON_MODE_FILL);
        assert_equal(mat->polygon_mode(), smlt::POLYGON_MODE_FILL);

        mat->set_polygon_mode(smlt::POLYGON_MODE_LINE);
        assert_equal(mat->polygon_mode(), smlt::POLYGON_MODE_LINE);
    }

    /* To save memory, we keep a global hash -> name mapping for all
     * properties across all materials. This test ensures we release
     * the elements from the map when it's no longer needed */
    void test_property_name_refcounting() {
        auto c0 = 0u;
        auto mat = application->shared_assets->new_material();
        mat->set_property_value("test", (int32_t) 1);

        auto c1 = Material::_name_refcount("test");

        assert_equal(c1, c0 + 1);

        auto mat2 = application->shared_assets->clone_material(mat);

        auto c2 = Material::_name_refcount("test");

        assert_equal(c2, c0 + 2);

        auto mat_count = application->shared_assets->material_count();
        mat.reset();
        application->shared_assets->run_garbage_collection();

        assert_equal(application->shared_assets->material_count(), mat_count - 1);

        auto c3 = Material::_name_refcount("test");

        assert_equal(c3, c0 + 1);

        mat2.reset();
        application->shared_assets->run_garbage_collection();

        assert_equal(application->shared_assets->material_count(), mat_count - 2);

        auto c4 = Material::_name_refcount("test");
        assert_equal(c4, c0);
    }
};


class MaterialCoreTest : public smlt::test::SimulantTestCase {
public:
    void test_is_core_property() {
        assert_true(is_core_property(DIFFUSE_PROPERTY_NAME));
        assert_true(is_core_property(AMBIENT_PROPERTY_NAME));
        assert_true(is_core_property(SPECULAR_PROPERTY_NAME));
        assert_true(is_core_property(SHININESS_PROPERTY_NAME));

        assert_false(is_core_property("my_property"));
    }

    void test_core_material_property_value() {
        const float* f;
        assert_true(core_material_property_value(SHININESS_PROPERTY_NAME, f));
        assert_equal(*f, 0.0f);

        assert_false(core_material_property_value("bananas", f));
        assert_false(core_material_property_value("s_diffuse", f));
    }

    void test_overriders() {
        MaterialPropertyOverrider o1;
        MaterialPropertyOverrider o2(&o1);

        const float* f;
        assert_true(o2.property_value(SHININESS_PROPERTY_NAME, f));
        assert_equal(*f, core_material().shininess);

        o1.set_property_value(SHININESS_PROPERTY_NAME, 1.5f);
        assert_true(o2.property_value(SHININESS_PROPERTY_NAME, f));
        assert_equal(*f, 1.5f);

        o2.set_property_value(SHININESS_PROPERTY_NAME, 2.5f);
        assert_true(o2.property_value(SHININESS_PROPERTY_NAME, f));
        assert_equal(*f, 2.5f);

        o2.clear_override(SHININESS_PROPERTY_NAME);
        assert_true(o2.property_value(SHININESS_PROPERTY_NAME, f));
        assert_equal(*f, 1.5f);
    }
};

}
