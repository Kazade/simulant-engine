#include "kglt/kglt.h"
#include "kglt/extra/skybox.h"

int main(int argc, char* argv[]) {        
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    if(argc < 2) {
        std::cout << "USAGE: model_viewer filename" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_logging_level(kglt::LOG_LEVEL_DEBUG);
    window->set_title("KGLT Model viewer");

    auto stage = window->stage();
    stage->set_ambient_light(kglt::Colour(1.0, 1.0, 1.0, 1.0));
    window->camera()->set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        1.0,
        1000.0
    );

    kglt::LightID light_id = stage->new_light();
    {
        auto light = stage->light(light_id);
        //light.set_direction(-1, 0, 0);
        light->set_diffuse(kglt::Colour::YELLOW);
        light->set_attenuation_from_range(100.0);
    }

    {
        auto light2 = stage->light(stage->new_light());
        light2->set_diffuse(kglt::Colour::RED);
        light2->set_attenuation_from_range(100.0);
        light2->move_to(20, -20, -50);
    }

    stage->set_ambient_light(kglt::Colour(0.2, 0.2, 0.2, 0.2));

    kglt::TextureID star_texture = stage->new_texture();
    auto tex = stage->texture(star_texture);

    kglt::procedural::texture::starfield(tex.__object);
    tex->upload();

    kglt::extra::SkyBox::ptr skybox = kglt::extra::SkyBox::create(stage, star_texture);

    kglt::MeshID mid = stage->new_mesh_from_file(filename);
    kglt::ActorID actor_id = stage->new_actor(mid);
    stage->actor(actor_id)->move_to(0, 0, -50);

    float x_position = 0.0f;
    bool incrementing = true;

    window->signal_step().connect(
        [&](double dt) {
            stage->actor(actor_id)->rotate_y(kglt::Degrees(10.0 * dt));

            x_position += ((incrementing) ? -10.0 : 10.0) * dt;

            if(x_position < -100) {
                x_position += 0.1;
                incrementing = !incrementing;
            }
            if(x_position > 100) {
                x_position -= 0.1;
                incrementing = !incrementing;
            }

            stage->light(light_id)->move_to(x_position, 20.0, -50.0);
        }
    );

    while(window->run_frame()) {}

    return 0;
}

