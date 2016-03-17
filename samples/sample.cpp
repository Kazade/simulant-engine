#include "kglt/kglt.h"

using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        kglt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        prepare_basic_scene(stage_id_, camera_id_);

        auto stage = window->stage(stage_id_);
        window->camera(camera_id_)->set_perspective_projection(
            45.0,
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );

        /**
            Generate a mesh and build a 2D square

            Base objects are always created with new_X() and can
            be destroyed with delete_X(). They are held by the object
            that spawned them. For example, meshes are held by the stage->

            Creating an object gives you an ID, this can then be exchanged
            for a reference to an object.
        */

        ///Shortcut function for loading images
        //kglt::TextureID tid = stage->new_texture_from_file("sample_data/sample.tga");
        kglt::TextureID tid = stage->new_texture_from_file("sample_data/q2/e1u2/metal9_3.wal");
        kglt::MaterialID matid = stage->new_material_from_texture(tid);

        stage->set_ambient_light(kglt::Colour::WHITE);
        stage->new_light(kglt::LIGHT_TYPE_DIRECTIONAL);

        actor_id_ = stage->new_actor_with_mesh(stage->new_mesh_as_capsule(0.5, 1.0));
        {
            auto actor = stage->actor(actor_id_);
            actor->mesh()->set_material_id(matid);
            actor->move_to(0.0f, 0.0f, -5.0f);
        }
    }

    void do_step(double dt) {
        auto stage = window->stage(stage_id_);
        stage->actor(actor_id_)->rotate_y(kglt::Degrees(20.0 * dt));
    }

private:
    CameraID camera_id_;
    StageID stage_id_;
    ActorID actor_id_;
};

class Sample: public kglt::Application {
public:
    Sample():
        Application("KGLT Combined Sample") {
    }

private:
    bool do_init() {
        register_screen("/", screen_factory<GameScreen>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    Sample app;
    return app.run();
}
