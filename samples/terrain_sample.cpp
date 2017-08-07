#include "simulant/utils/random.h"
#include "simulant/simulant.h"
#include "simulant/scenes/loading.h"

using namespace smlt;

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

        float rock = clamp(steepness.value / 45.0f);
        float sand = clamp(1.0 - (height * 4.0f));
        float grass = (sand > 0.5) ? 0.0 : 0.5f;
        float snow = height * clamp(n.z);

        float z = rock + sand + grass + snow;

        texture->data()[i * 4] = 255.0f * (sand / z);
        texture->data()[(i * 4) + 1] = 255.0f * (grass / z);
        texture->data()[(i * 4) + 2] = 255.0f * (rock / z);
        texture->data()[(i * 4) + 3] = 255.0f * (snow / z);
    }
    texture->upload(smlt::MIPMAP_GENERATE_NONE, smlt::TEXTURE_WRAP_CLAMP_TO_EDGE, smlt::TEXTURE_FILTER_LINEAR, false);
}

class Gamescene : public smlt::Scene<Gamescene> {
public:
    Gamescene(smlt::WindowBase& window):
        smlt::Scene<Gamescene>(window) {}

    void load() {
        auto loading = window->application->resolve_scene_as<scenes::Loading>("_loading");
        assert(loading);

        bool done = false;

        // While we're loading, continually pulse the progress bar to show that stuff is happening
        window->idle->add([&loading, &done]() {
            loading->progress_bar->pulse();
            return !done;
        });

        pipeline_id_ = prepare_basic_scene(stage_id_, camera_id_);
        window->disable_pipeline(pipeline_id_);

        auto stage = window->stage(stage_id_);

        camera_id_.fetch()->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 10.0, 10000.0
        );

        window->pipeline(pipeline_id_)->viewport->set_colour(smlt::Colour::SKY_BLUE);

        auto cam = stage->camera(camera_id_);
        cam->move_to(0, 50, 700);
        cam->look_at(0, 0, 0);

        terrain_material_id_ = stage->assets->new_material_from_file("sample_data/terrain_splat.kglm", GARBAGE_COLLECT_NEVER);
        smlt::HeightmapSpecification spec;
        spec.smooth_iterations = 0;

        terrain_mesh_id_ = stage->assets->new_mesh_from_heightmap("sample_data/terrain.png", spec);
        auto terrain_mesh = stage->assets->mesh(terrain_mesh_id_);

        auto terrain_data = terrain_mesh->data->get<smlt::TerrainData>("terrain_data");
        smlt::TextureID terrain_splatmap = stage->assets->new_texture();
        calculate_splat_map(
            terrain_data.x_size,
            terrain_data.z_size,
            stage->assets->texture(terrain_splatmap),
            terrain_mesh->shared_data
        );

        stage->assets->material(terrain_material_id_)->first_pass()->set_texture_unit(4, terrain_splatmap);

        terrain_mesh->set_material_id(terrain_material_id_);

        terrain_actor_id_ = stage->new_actor_with_mesh(terrain_mesh_id_);

        done = true;
    }

    void activate() {
        window->enable_pipeline(pipeline_id_);
    }

    void fixed_update(float dt) override {
        auto stage = window->stage(stage_id_);
        stage->actor(terrain_actor_id_)->rotate_global_y_by(smlt::Degrees(dt * 5.0));
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


class TerrainDemo: public smlt::Application {
public:
    TerrainDemo(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        register_scene<Gamescene>("main");
        load_scene_in_background("main", true); //Do loading in a background thread, but show immediately when done
        activate_scene("_loading"); // Show the loading scene in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Terrain Demo";

    TerrainDemo app(config);
    return app.run();
}
