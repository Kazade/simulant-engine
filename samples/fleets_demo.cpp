#include "simulant/simulant.h"
#include "simulant/utils/random.h"

using namespace smlt;

class GameScreen : public smlt::Screen<GameScreen> {
public:
    GameScreen(smlt::WindowBase& window):
        smlt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        auto pipeline = prepare_basic_scene(stage_id_, camera_id_);
        pipeline.fetch()->viewport->set_colour(smlt::Colour::BLACK);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);
        window->camera(camera_id_)->set_perspective_projection(45.0, float(window->width()) / float(window->height()), 10.0, 10000.0);
        ship_mesh_id_ = window->shared_assets->new_mesh_from_file("sample_data/fighter_good/space_frigate_6.obj");
        ship_mesh_id_.fetch()->reverse_winding();
        generate_ships();

        stage->set_ambient_light(smlt::Colour(0.2, 0.2, 0.2, 1.0));
        stage->new_light_as_directional();
    }

    void do_activate() {

    }

    void fixed_update(double dt) override {
        Vec3 speed(-250, 0, 0.0);

        Vec3 avg;

        auto stage = window->stage(stage_id_);
        for(auto ship_id: ship_ids_) {
            auto pos = stage->actor(ship_id)->position();
            avg += pos;
            stage->actor(ship_id)->move_to_absolute(pos + (speed * dt * (0.01 * ship_id.value())));
        }

        avg /= ship_ids_.size();

        stage->camera(camera_id_)->look_at(avg);
    }

private:
    StageID stage_id_;
    CameraID camera_id_;

    MeshID ship_mesh_id_;
    std::vector<ActorID> ship_ids_;

    void generate_ships() {
        Vec3 centre = Vec3(2000, 0, 0);

        for(uint32_t i = 0; i < 100; ++i) {
            Vec3 pos = centre + (Vec3(
                random_gen::random_float(-100, 100),
                random_gen::random_float(-100, 100),
                random_gen::random_float(-100, 100)
            ).normalized() * random_gen::random_float(100.0f, 150.0f));

            auto stage = window->stage(stage_id_);
            ship_ids_.push_back(stage->new_actor_with_mesh(ship_mesh_id_));
            stage->actor(ship_ids_.back())->move_to_absolute(pos);

            auto ps_id = stage->new_particle_system_with_parent_from_file(ship_ids_.back(), "simulant/particles/pixel_trail.kglp");
            stage->particle_system(ps_id)->move_to(0, 0, 0);
        }
    }
};


class FleetsDemo: public smlt::Application {
public:
    FleetsDemo():
        smlt::Application("Fleets Demo") {}

private:
    bool do_init() {
        register_screen("/", smlt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    FleetsDemo app;
    return app.run();
}
