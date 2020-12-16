#include "simulant/simulant.h"
#include "simulant/scenes/loading.h"

using namespace smlt;

template<typename T>
inline T clamp(T x, T a = 0, T b = 1) {
    return x < a ? a : (x > b ? b : x);
}

void calculate_splat_map(int width, int length, TexturePtr texture, VertexData& vertices) {
    texture->resize(width, length);
    texture->mutate_data([&](uint8_t* data, uint16_t, uint16_t, TextureFormat) {
        for(uint32_t i = 0; i < vertices.count(); ++i) {
            auto n = vertices.normal_at<Vec3>(i);

            Degrees steepness = Radians(acos(n->dot(Vec3(0, 1, 0))));
            float height = (vertices.position_at<Vec3>(i)->y + 64.0f) / 128.0f;
            float rock = clamp(steepness.value / 45.0f);
            float sand = clamp(1.0f - (height * 4.0f));
            float grass = (sand > 0.5f) ? 0.0f : 0.5f;
            float snow = height * clamp(n->z);

            float z = rock + sand + grass + snow;

            data[i * 4] = 255.0f * (sand / z);
            data[(i * 4) + 1] = 255.0f * (grass / z);
            data[(i * 4) + 2] = 255.0f * (rock / z);
            data[(i * 4) + 3] = 255.0f * (snow / z);
        }
    });
}

class Gamescene : public smlt::Scene<Gamescene> {
public:
    Gamescene(smlt::Window* window):
        smlt::Scene<Gamescene>(window) {}

    void load() override {
        auto loading = scenes->resolve_scene_as<scenes::Loading>("_loading");
        assert(loading);

        auto done = std::make_shared<bool>(false);

        // While we're loading, continually pulse the progress bar to show that stuff is happening
        window->idle->add([this, loading, done]() {
            if(!scenes->has_scene("_loading")) {
                return false;
            }

            if(loading->is_loaded() && loading->progress_bar) {
                loading->progress_bar->pulse();
            }
            return !(*done);
        });

        stage_ = window->new_stage(smlt::PARTITIONER_NULL);
        camera_ = stage_->new_camera();
        pipeline_ = compositor->render(
            stage_, camera_
        )->set_clear_flags(
            smlt::BUFFER_CLEAR_ALL
        );
        pipeline_->viewport->set_colour(smlt::Colour::SKY_BLUE);

        link_pipeline(pipeline_);

        camera_->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 10.0, 10000.0
        );

        auto cam = camera_;
        cam->move_to(0, 50, 700);
        cam->look_at(0, 0, 0);

        cam->new_behaviour<smlt::behaviours::Fly>(window);

        auto terrain_material = stage_->assets->new_material_from_file(
            "sample_data/terrain_splat.smat", GARBAGE_COLLECT_NEVER
        );

        terrain_material_id_ = terrain_material;

        smlt::HeightmapSpecification spec;
        spec.smooth_iterations = 0;

        terrain_mesh_id_ = stage_->assets->new_mesh_from_heightmap("sample_data/terrain.png", spec);
        auto terrain_mesh = stage_->assets->mesh(terrain_mesh_id_);

        auto terrain_data = terrain_mesh->data->get<smlt::TerrainData>("terrain_data");
        auto terrain_splatmap = stage_->assets->new_texture(terrain_data.x_size, terrain_data.z_size);
        calculate_splat_map(
            terrain_data.x_size,
            terrain_data.z_size,
            terrain_splatmap,
            terrain_mesh->vertex_data
        );

        terrain_material->pass(0)->set_property_value(
            terrain_material->find_property_id("textures[4]"),
            terrain_splatmap
        );

        terrain_mesh->set_material(terrain_material);

        GeomCullerOptions opts;
        opts.type = GEOM_CULLER_TYPE_QUADTREE;
        terrain_actor_ = stage_->new_geom_with_mesh(terrain_mesh_id_, opts);

        *done = true;
    }

    void fixed_update(float) override {}

private:
    PipelinePtr pipeline_;
    StagePtr stage_;
    CameraPtr camera_;

    MeshID terrain_mesh_id_;
    GeomPtr terrain_actor_;
    MaterialID terrain_material_id_;

    TextureID terrain_textures_[4];
};


class TerrainDemo: public smlt::Application {
public:
    TerrainDemo(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<Gamescene>("main");
        scenes->preload_in_background("main", true); //Do loading in a background thread, but show immediately when done
        scenes->load_and_activate("_loading"); // Show the loading scene in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Terrain Demo";
    config.fullscreen = false;
    config.width = 640 * 2;
    config.height = 480 * 2;

    TerrainDemo app(config);
    return app.run();
}
