#include "simulant/simulant.h"

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

class Gamescene : public smlt::Scene {
public:
    Gamescene(smlt::Window* window):
        smlt::Scene(window) {}

    void load() override {
        stage_ = create_node<smlt::Stage>();
        camera_ = create_node<smlt::Camera>();
        pipeline_ = compositor->render(
            stage_, camera_
        )->set_clear_flags(
            smlt::BUFFER_CLEAR_ALL
        );
        pipeline_->viewport->set_colour(smlt::Colour::BLUE);

        link_pipeline(pipeline_);

        camera_->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 10.0, 10000.0f
        );

        auto cam = camera_;
        cam->move_to(0, 50, 700);
        cam->look_at(0, 0, 0);

        auto fly_node = create_node<smlt::FlyController>();
        cam->set_parent(fly_node);

        auto terrain_material = assets->new_material_from_file(
            "sample_data/terrain_splat.smat", GARBAGE_COLLECT_NEVER
        );

        terrain_material_ = terrain_material;

        smlt::HeightmapSpecification spec;
        spec.smooth_iterations = 0;

        terrain_mesh_ = assets->new_mesh_from_heightmap("sample_data/terrain.png", spec);

        auto terrain_data = terrain_mesh_->data->get<smlt::TerrainData>("terrain_data");
        auto terrain_splatmap = assets->new_texture(terrain_data.x_size, terrain_data.z_size);
        calculate_splat_map(
            terrain_data.x_size,
            terrain_data.z_size,
            terrain_splatmap,
            terrain_mesh_->vertex_data
        );

        terrain_material->pass(0)->set_property_value(
            "textures[4]",
            terrain_splatmap
        );

        terrain_mesh_->set_material(terrain_material);

        GeomCullerOptions opts;
        opts.type = GEOM_CULLER_TYPE_QUADTREE;
        terrain_actor_ = create_node<smlt::Geom>(terrain_mesh_, opts);
    }

    void on_fixed_update(float) override {}

private:
    PipelinePtr pipeline_;
    StagePtr stage_;
    CameraPtr camera_;

    MeshPtr terrain_mesh_;
    GeomPtr terrain_actor_;
    MaterialPtr terrain_material_;
    TexturePtr terrain_textures_[4];
};


class TerrainDemo: public smlt::Application {
public:
    TerrainDemo(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<Gamescene>("main");
        scenes->preload_in_background("main").then([this](){
            scenes->activate("main");
        }); //Do loading in a background thread, but show immediately when done
        scenes->activate("_loading"); // Show the loading scene in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Terrain Demo";
    config.fullscreen = false;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    TerrainDemo app(config);
    return app.run();
}
