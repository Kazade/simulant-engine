#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    window->set_title("Lighting Sample");
    window->scene().camera()->set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        0.1,
        1000.0
    );


    auto stage = window->stage();
    stage->set_ambient_light(kglt::Colour(0.2, 0.2, 0.2, 1.0));

    kglt::ActorID actor_id = stage->geom_factory().new_cube(2.0);
    stage->actor(actor_id)->move_to(0.0, 0.0, -10.0);

    kglt::TextureID texture = stage->new_texture_from_file("sample_data/crate.png");
    stage->actor(actor_id)->mesh()->set_texture_on_material(0, texture);

    // Test Camera::look_at function
    stage->camera()->look_at(stage->actor(actor_id)->absolute_position());

    {
        auto light = stage->light(stage->new_light());
        light->move_to(5.0, 0.0, -5.0);
        light->set_diffuse(kglt::Colour::GREEN);
        light->set_attenuation_from_range(5.0);

        auto light2 = stage->light(stage->new_light());
        light2->move_to(-5.0, 0.0, -5.0);
        light2->set_diffuse(kglt::Colour::BLUE);
        light2->set_attenuation_from_range(10.0);

        auto light3 = stage->light(stage->new_light());
        light3->move_to(0.0, 15.0, -5.0);
        light3->set_diffuse(kglt::Colour::RED);
        light3->set_attenuation_from_range(20.0);
    }

    float xpos = 0;
    window->keyboard().key_while_pressed_connect(SDL_SCANCODE_A, [&](SDL_Keysym key, double dt) mutable {
            xpos -= 20.0 * dt;
            window->stage()->camera()->set_absolute_position(xpos, 2, 0);
            window->stage()->camera()->look_at(window->stage()->actor(actor_id)->absolute_position());
    });
    window->keyboard().key_while_pressed_connect(SDL_SCANCODE_D, [&](SDL_Keysym key, double dt) mutable {
            xpos += 20.0 * dt;
            window->stage()->camera()->set_absolute_position(xpos, 2, 0);
            window->stage()->camera()->look_at(stage->actor(actor_id)->absolute_position());
    });




    while(window->run_frame()) {
        window->stage()->actor(actor_id)->rotate_x(kglt::Degrees(window->delta_time() * 20.0));
        window->stage()->actor(actor_id)->rotate_y(kglt::Degrees(window->delta_time() * 15.0));
        window->stage()->actor(actor_id)->rotate_z(kglt::Degrees(window->delta_time() * 25.0));
    }

    return 0;
}
