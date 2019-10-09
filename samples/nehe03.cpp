
#include <simulant/simulant.h>

class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() {
        stage_ = window->new_stage();
        camera_ = stage_->new_camera();
        auto pipeline = window->render(stage_, camera_);
        link_pipeline(pipeline);

        smlt::MeshPtr square = stage_->assets->new_mesh_as_rectangle(1.0, 1.0);

        square->vertex_data->move_to(0);
        square->vertex_data->diffuse(smlt::Colour::RED);
        square->vertex_data->move_next();

        square->vertex_data->diffuse(smlt::Colour::GREEN);
        square->vertex_data->move_next();

        square->vertex_data->diffuse(smlt::Colour::BLUE);
        square->vertex_data->move_next();

        square->vertex_data->diffuse(smlt::Colour::YELLOW);
        square->vertex_data->move_to_end();
        square->vertex_data->done();

        auto actor = stage_->new_actor_with_mesh(square->id());
        actor->move_to(0, 0, -5);
        actor->scale_by(2.0);

        L_DEBUG("Scene loaded");
    }

private:
    smlt::StagePtr stage_;
    smlt::CameraPtr camera_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

        window->set_logging_level(smlt::LOG_LEVEL_DEBUG);
    }

    bool init() {
        scenes->register_scene<MainScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "NeHe 01";
    App app(config);
    return app.run();
}
