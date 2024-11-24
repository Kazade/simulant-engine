#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

#include "simulant/assets/materials/core/core_material.h"
#include "simulant/assets/materials/core/material_property_overrider.h"
#include "simulant/utils/random.h"

namespace {

using namespace smlt;

class MaterialTest : public smlt::test::SimulantTestCase {
public:
    void test_material_initialization() {
        auto mat = application->shared_assets->create_material();

        mat->set_pass_count(1);

        this->assert_equal((uint32_t)1, mat->pass_count()); //Should return the default pass
        this->assert_true(
            smlt::Color::white() ==
            mat->pass(0)->diffuse()); // this->assert_true the default pass sets
                                      // white as the default
        this->assert_true(
            smlt::Color::white() ==
            mat->pass(0)->ambient()); // this->assert_true the default pass sets
                                      // white as the default
        this->assert_true(
            smlt::Color::black() ==
            mat->pass(0)->specular()); // this->assert_true the default pass
                                       // sets black as the default
        this->assert_equal(0.0f, mat->pass(0)->shininess());
    }

    void test_material_applies_to_mesh() {
        auto mat = application->shared_assets->create_material();
        auto mesh = application->shared_assets->create_mesh(smlt::VertexSpecification::POSITION_ONLY);
        smlt::SubMesh* sm = mesh->create_submesh("test", mat);
        this->assert_equal(mat->id(), sm->material()->id());
    }

    void test_property_heirarchy() {
        auto mat = application->shared_assets->create_material();

        mat->set_diffuse(smlt::Color::red());
        mat->set_pass_count(2);

        auto pass1 = mat->pass(0);
        auto pass2 = mat->pass(1);

        assert_equal(pass1->diffuse(), smlt::Color::red());
        assert_equal(pass2->diffuse(), smlt::Color::red());

        pass1->set_diffuse(smlt::Color::green());

        assert_equal(pass1->diffuse(), smlt::Color::green());
        assert_equal(pass2->diffuse(), smlt::Color::red());
    }

    void test_material_property_stress_test() {
        int iterations = 100;

        auto mat = application->shared_assets->create_material();
        mat->set_pass_count(2);
        auto p0 = mat->pass(0);
        auto& r = RandomGenerator::instance();

        struct Expected {
            std::string name;
            optional<Color> color;
            optional<bool> b;

            Expected() = default;
            Expected(const std::string& name, const optional<Color>& color,
                     const optional<bool>& b) :
                name(name), color(color), b(b) {}
        };

        std::vector<std::string> color_props = {DIFFUSE_PROPERTY_NAME,
                                                AMBIENT_PROPERTY_NAME,
                                                SPECULAR_PROPERTY_NAME};

        std::vector<std::string> bool_props = {
            DEPTH_TEST_ENABLED_PROPERTY_NAME, DEPTH_WRITE_ENABLED_PROPERTY_NAME,
            LIGHTING_ENABLED_PROPERTY_NAME};

        std::vector<Expected> expected;

        for(int i = 0; i < iterations; ++i) {
            int type = r.int_in_range(0, 2);
            switch(type) {
                case 0: // Color
                {
                    auto v = r.point_on_sphere(1.0f);
                    auto c = Color(v.x, v.y, v.z, 1.0);
                    auto p = r.choice(color_props);

                    printf("Set %s to %f, %f, %f, %f\n", p.c_str(), c.r, c.g,
                           c.b, c.a);
                    p0->set_property_value(p.c_str(), c);

                    optional<Color> ov = c;

                    expected.erase(std::remove_if(expected.begin(),
                                                  expected.end(),
                                                  [&](const Expected& e) {
                        return e.name == p;
                    }),
                                   expected.end());
                    expected.push_back(Expected(p, ov, optional<bool>()));
                    break;
                }
                case 1: // Bool
                {
                    auto v = r.int_in_range(0, 1) == 1;
                    auto p = r.choice(bool_props);
                    p0->set_property_value(p.c_str(), v);
                    printf("Set %s to %d\n", p.c_str(), v);
                    optional<bool> ov = v;
                    expected.erase(std::remove_if(expected.begin(),
                                                  expected.end(),
                                                  [&](const Expected& e) {
                        return e.name == p;
                    }),
                                   expected.end());
                    expected.push_back(Expected(p, optional<Color>(), ov));
                    break;
                }
                case 2: // Remove random
                {
                    r.shuffle(expected);
                    auto remove_count = r.int_in_range(0, expected.size());
                    for(int i = 0; i < remove_count; ++i) {
                        auto& e = expected[expected.size() - i - 1];
                        printf("Cleared %s\n", e.name.c_str());
                        p0->clear_override(e.name.c_str());
                    }

                    expected.resize(expected.size() - remove_count);
                    break;
                }
            }

            for(auto& e: expected) {
                if(e.color) {
                    const Color* out;
                    assert_true(p0->property_value(e.name.c_str(), out));
                    auto t = e.color.value();
                    if(t != *out) {
                        printf(
                            "Failed: %s -> %f, %f, %f, %f vs %f, %f, %f, %f\n",
                            e.name.c_str(), out->r, out->g, out->b, out->a, t.r,
                            t.g, t.b, t.a);
                    }
                    assert_equal(t, *out);
                } else {
                    const bool* out;
                    assert_true(p0->property_value(e.name.c_str(), out));

                    bool t = e.b.value();
                    if(t != *out) {
                        printf("Failed: %s -> %d vs %d\n", e.name.c_str(), *out,
                               t);
                    }
                    assert_equal(t, *out);
                }
            }
        }
    }

    void test_pass_resizing() {
        auto mat1 = application->shared_assets->create_material();

        // Materials have a single pass by default, rightly or wrongly...
        assert_equal(mat1->pass_count(), 1);

        mat1->set_pass_count(2);
        assert_equal(mat1->pass_count(), 2);

        mat1->set_pass_count(1);
        assert_equal(mat1->pass_count(), 1);

        mat1->set_pass_count(2);
        assert_equal(mat1->pass_count(), 2);

        auto mat2 = application->shared_assets->clone_material(mat1->id());

        assert_equal(mat2->pass_count(), 2);
    }

    void test_material_copies() {
        auto mat1 = application->shared_assets->create_material();
        auto tex1 = application->shared_assets->create_texture(8, 8);

        mat1->set_diffuse(smlt::Color::red());
        mat1->set_diffuse_map(tex1);

        mat1->set_pass_count(2);
        mat1->pass(0)->set_diffuse(smlt::Color::blue());

        auto mat2 = application->shared_assets->clone_material(mat1->id());

        assert_not_equal(mat1->id(), mat2->id());

        assert_equal(mat1->diffuse_map(), tex1);
        assert_equal(mat1->diffuse(), smlt::Color::red());
        assert_equal(mat1->pass_count(), 2);
        assert_equal(mat1->pass(0)->diffuse(), smlt::Color::blue());
        assert_equal(mat1->pass(1)->diffuse(), smlt::Color::red());
        assert_equal(mat1->pass(0)->diffuse_map(), tex1);

        // Make sure the passes were copied
        assert_not_equal(mat1->pass(0), mat2->pass(0));

        assert_equal(mat2->diffuse_map(), tex1);
        assert_equal(mat2->diffuse(), smlt::Color::red());
        assert_equal(mat2->pass_count(), 2);
        assert_equal(mat2->pass(0)->diffuse(), smlt::Color::blue());
        assert_equal(mat2->pass(1)->diffuse(), smlt::Color::red());
        assert_equal(mat2->pass(0)->diffuse_map(), tex1);

        mat2->set_diffuse(smlt::Color::green());
        assert_equal(mat2->pass(1)->diffuse(), smlt::Color::green());
    }

    void test_texture_unit() {
        auto mat = application->shared_assets->create_material();
        auto tex = application->shared_assets->create_texture(8, 8);

        mat->set_diffuse_map(tex);
        mat->set_pass_count(2);

        auto pass1 = mat->pass(0);
        auto pass2 = mat->pass(1);

        assert_equal(pass1->diffuse_map(), tex);
        assert_equal(pass2->diffuse_map(), tex);

        auto tex2 = application->shared_assets->create_texture(8, 8);

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
        auto mat = application->shared_assets->create_material();

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

        auto mat = application->shared_assets->create_material();
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
        auto mat = application->shared_assets->create_material();
        mat->set_pass_count(1);

        auto texture = application->shared_assets->create_texture(8, 8);
        assert_equal(texture.use_count(), 2);

        mat->set_diffuse_map(texture);

        assert_equal(mat->diffuse_map(), texture);

        assert_equal(texture.use_count(), 3);
    }

    // FIXME: Restore this
    void test_reflectiveness() {
        /*
        smlt::MaterialID mid = application->shared_assets->create_material();
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
        smlt::MaterialPtr mat = application->shared_assets->create_material();
        mat->set_polygon_mode(smlt::POLYGON_MODE_FILL);
        assert_equal(mat->polygon_mode(), smlt::POLYGON_MODE_FILL);

        mat->set_polygon_mode(smlt::POLYGON_MODE_LINE);
        assert_equal(mat->polygon_mode(), smlt::POLYGON_MODE_LINE);
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

    void test_overriders() {
        auto o1 = application->shared_assets->create_material();
        o1->set_pass_count(1);
        auto o2 = *o1->pass(0);

        const float* f;
        o1->set_property_value(SHININESS_PROPERTY_NAME, 1.5f);
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
