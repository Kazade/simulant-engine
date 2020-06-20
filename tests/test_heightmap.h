#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class HeightmapTests : public test::SimulantTestCase {
public:
    void test_basic_usage() {
        auto stage = window->new_stage();
        auto path = "flare.tga";
        auto tex = stage->assets->new_texture_from_file(path);
        auto heightmap = stage->assets->new_mesh_from_heightmap(path, HeightmapSpecification());

        assert_equal(heightmap->data->get<TerrainData>("terrain_data").x_size, tex->width());
        assert_equal(heightmap->data->get<TerrainData>("terrain_data").z_size, tex->height());
    }

    void test_height_at_xz() {
        uint8_t heightmap_data [] = {
            0, 128, 255, 0,
            0, 128, 255, 0,
            0, 128, 255, 0,
            0, 128, 255, 0,
        };

        HeightmapSpecification spec;

        auto stage = window->new_stage();

        auto tex = stage->assets->new_texture(4, 4, TEXTURE_FORMAT_R8);
        tex->set_auto_upload(false);
        tex->set_data(heightmap_data);
        auto mesh = stage->assets->new_mesh_from_heightmap(tex, spec);

        auto data = mesh->data->get<TerrainData>("terrain_data");
        assert_equal(data.x_size, 4u);
        assert_equal(data.z_size, 4u);

        /* Outside the bounds */
        assert_false(data.height_at_xz(Vec2(5 * spec.spacing / 2, 5 * spec.spacing / 2)));

        float hw = spec.spacing * 2;

        auto normalized_height = [spec](float n) -> float {
            return spec.min_height + (spec.max_height - spec.min_height) * n;
        };

        assert_close(
            data.height_at_xz(Vec2((0 * spec.spacing) - hw, 0)).value(),
            normalized_height(0.0f), 0.001f
        );

        assert_close(
            data.height_at_xz(Vec2((1 * spec.spacing) - hw, 0)).value(),
            normalized_height(0.5f), 0.001f
        );

        assert_close(
            data.height_at_xz(Vec2((2 * spec.spacing) - hw, 0)).value(),
            normalized_height(1.0f), 0.5f
        );

        assert_close(
            data.height_at_xz(Vec2((3 * spec.spacing) - hw, 0)).value(),
            normalized_height(0.0f), 0.001f
        );
    }

    void test_triangle_at_xz() {
        uint8_t heightmap_data [] = {
            0, 128, 255, 0,
            0, 128, 255, 0,
            0, 128, 255, 0,
            0, 128, 255, 0,
        };

        HeightmapSpecification spec;

        auto stage = window->new_stage();

        auto tex = stage->assets->new_texture(4, 4, TEXTURE_FORMAT_R8);
        tex->set_auto_upload(false);
        tex->set_data(heightmap_data);
        auto mesh = stage->assets->new_mesh_from_heightmap(tex, spec);

        auto data = mesh->data->get<TerrainData>("terrain_data");

        auto tri = data.triangle_at_xz(-10.0f, -10.0f).value();

        assert_equal(tri.index[0], 0u);
        assert_equal(tri.index[1], 2u);
        assert_equal(tri.index[2], 1u);

        tri = data.triangle_at_xz(-8.0f, -10.0f).value();

        assert_equal(tri.index[0], 0u);
        assert_equal(tri.index[1], 2u);
        assert_equal(tri.index[2], 1u);

        tri = data.triangle_at_xz(-8.0f, -8.0f).value();

        assert_equal(tri.index[0], 2u);
        assert_equal(tri.index[1], 3u);
        assert_equal(tri.index[2], 1u);
    }
};

}
