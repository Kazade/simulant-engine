#include "kglt/kglt.h"

using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        kglt::Screen<GameScreen>(window) {}

    void do_load() {
        camera_id_ = window->new_camera();
        cube_stage_ = window->new_stage();
        rect_stage_ = window->new_stage();

        kglt::TextureID tid = window->new_texture_from_file("sample_data/sample.tga");
        MeshID cube_mesh = window->new_mesh_as_cube(1.0);
        window->mesh(cube_mesh)->set_texture_on_material(0, tid);
        cube_ = window->stage(cube_stage_)->new_actor_with_mesh(cube_mesh);
        window->stage(cube_stage_)->actor(cube_)->set_absolute_position(0, 0, -4);

        MeshID rect_mesh = window->new_mesh_as_rectangle(2.0, 2.0);
        rect_ = window->stage(rect_stage_)->new_actor_with_mesh(rect_mesh);
        window->stage(rect_stage_)->actor(rect_)->set_absolute_position(0, 0, -4);

        TextureID rtt = window->new_texture(false);
        window->mesh(rect_mesh)->set_texture_on_material(0, rtt);

        window->render(cube_stage_, camera_id_).to_texture(rtt);
        window->render(rect_stage_, camera_id_).to_framebuffer(
            Viewport(VIEWPORT_TYPE_FULL, kglt::Colour::GREY)
        ).with_clear();
    }

    void do_step(double dt) {
        window->stage(cube_stage_)->actor(cube_)->rotate_y(Degrees(dt * 360));
        window->stage(rect_stage_)->actor(rect_)->rotate_y(Degrees(dt * 180));
    }

private:
    CameraID camera_id_;
    StageID cube_stage_, rect_stage_;
    ActorID cube_, rect_;
};

class Sample: public kglt::Application {
public:
    Sample():
        Application("KGLT RTT Sample") {
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
