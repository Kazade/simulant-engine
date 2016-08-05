#include <kazbase/random.h>

#include "kglt/kglt.h"

using namespace kglt;

template<typename T>
inline T clamp(T x, T a = 0, T b = 1) {
    return x < a ? a : (x > b ? b : x);
}

void calculate_splat_map(int width, int length, TexturePtr texture, VertexData& vertices) {
    texture->resize(width, length);

    for(uint32_t i = 0; i < vertices.count(); ++i) {
        Vec3 n;
        vertices.normal_at(i, n);

        Degrees steepness = Radians(acos(n.dot(Vec3(0, 1, 0))));
        float height = (vertices.position_at<Vec3>(i).y + 64.0f) / 128.0f;

        float rock = clamp(steepness.value_ / 45.0f);
        float sand = clamp(1.0 - (height * 4.0f));
        float grass = (sand > 0.5) ? 0.0 : 0.5f;
        float snow = height * clamp(n.z);

        float z = rock + sand + grass + snow;

        texture->data()[i * 4] = 255.0f * (sand / z);
        texture->data()[(i * 4) + 1] = 255.0f * (grass / z);
        texture->data()[(i * 4) + 2] = 255.0f * (rock / z);
        texture->data()[(i * 4) + 3] = 255.0f * (snow / z);
    }
    texture->upload(kglt::MIPMAP_GENERATE_NONE, kglt::TEXTURE_WRAP_CLAMP_TO_EDGE, kglt::TEXTURE_FILTER_LINEAR, false);
}

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(kglt::WindowBase& window):
        kglt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        pipeline_id_ = prepare_basic_scene(stage_id_, camera_id_, kglt::PARTITIONER_NULL);
        window->disable_pipeline(pipeline_id_);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);
        window->camera(camera_id_)->set_perspective_projection(
            45.0, float(window->width()) / float(window->height()), 10.0, 10000.0
        );

        window->pipeline(pipeline_id_)->viewport->set_colour(kglt::Colour::SKY_BLUE);

        auto cam = stage->camera(camera_id_);
        cam->move_to(0, 50, 700);
        cam->look_at(0, 0, 0);

        terrain_material_id_ = stage->resources->new_material_from_file("sample_data/terrain_splat.kglm", GARBAGE_COLLECT_NEVER);
        kglt::HeightmapSpecification spec;
        spec.smooth_iterations = 0;

        terrain_mesh_id_ = stage->resources->new_mesh_from_heightmap("sample_data/terrain.png", spec);
        auto terrain_mesh = stage->resources->mesh(terrain_mesh_id_);

        auto terrain_data = terrain_mesh->get<kglt::TerrainData>("terrain_data");
        kglt::TextureID terrain_splatmap = stage->resources->new_texture();
        calculate_splat_map(
            terrain_data.x_size,
            terrain_data.z_size,
            stage->resources->texture(terrain_splatmap),
            terrain_mesh->shared_data
        );

        stage->resources->material(terrain_material_id_)->first_pass()->set_texture_unit(4, terrain_splatmap);

        terrain_mesh->set_material_id(terrain_material_id_);

        terrain_actor_id_ = stage->new_actor_with_mesh(terrain_mesh_id_);
    }

    void do_activate() {
        window->enable_pipeline(pipeline_id_);
    }

    void do_step(double dt) override {
        auto stage = window->stage(stage_id_);
        stage->actor(terrain_actor_id_)->rotate_global_y(kglt::Degrees(dt * 5.0));
    }

private:
    PipelineID pipeline_id_;
    StageID stage_id_;
    CameraID camera_id_;

    MeshID terrain_mesh_id_;
    ActorID terrain_actor_id_;
    MaterialID terrain_material_id_;

    TextureID terrain_textures_[4];
};


class TerrainDemo: public kglt::Application {
public:
    TerrainDemo():
        kglt::Application("Terrain Demo") {}

private:
    bool do_init() {
        register_screen("/", kglt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    TerrainDemo app;
    return app.run();
}
