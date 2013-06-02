#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/kazbase/string.h"
#include "kglt/extra/skybox.h"

int main(int argc, char* argv[]) {        
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    if(argc < 2) {
        std::cout << "USAGE: flightsim filename" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    if(!str::contains(str::lower(filename), ".opt")) {
        std::cout << "Please specify a file with a .opt extension" << std::endl;
        return 2;
    }

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_title("Flight Sim Camera");

    kglt::Stage& stage = window->scene().stage();
    stage.set_ambient_light(kglt::Colour(1.0, 1.0, 1.0, 1.0));
    window->scene().camera().set_perspective_projection(45.0, float(window->width()) / float(window->height()));

    kglt::MeshID mid = stage.new_mesh_from_file(filename);
    kglt::Actor& actor = stage.actor(stage.new_actor(mid));
    actor.move_to(0, 0, -30);

    //Just stash the skybox along with the scene
    window->scene().data().stash(kglt::extra::StarField::create(stage), "skybox");

    //Set the camera to follow the model
    window->scene().camera().follow(stage.actor_ref(actor.id()), kglt::Vec3(0, 5, 50));


    //Connect keyboard signals
    window->keyboard().key_while_down_connect(kglt::KEY_CODE_LEFT, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).rotate_y(-20.0 * dt);
    });

    window->keyboard().key_while_down_connect(kglt::KEY_CODE_RIGHT, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).rotate_y(20.0 * dt);
    });

    window->keyboard().key_while_down_connect(kglt::KEY_CODE_UP, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).rotate_x(20.0 * dt);
    });

    window->keyboard().key_while_down_connect(kglt::KEY_CODE_DOWN, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).rotate_x(-20.0 * dt);
    });

    window->keyboard().key_while_down_connect(kglt::KEY_CODE_a, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).rotate_z(-60.0 * dt);
    });

    window->keyboard().key_while_down_connect(kglt::KEY_CODE_d, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).rotate_z(60.0 * dt);
    });

    window->keyboard().key_while_down_connect(kglt::KEY_CODE_s, [=](kglt::KeyEvent key, double dt) mutable {
        window->scene().stage().actor(actor.id()).move_forward(60 * dt);
    });

    /*if(window->joypad_count()) {
        window->joypad(0).axis_while_nonzero_connect(0, [=](kglt::AxisRange range, kglt::Axis) mutable {
            float dt = window->delta_time();
            window->scene().stage().actor(actor.id()).rotate_y((range * 30) * dt);
        });

        window->joypad(0).axis_while_nonzero_connect(1, [=](kglt::AxisRange range, kglt::Axis) mutable {
            float dt = window->delta_time();
            window->scene().stage().actor(actor.id()).rotate_x((range * -30) * dt);
        });
    }*/

    while(window->update()) {}

    return 0;
}

