#include <kglt/kglt.h>
#include <kglt/shortcuts.h>
#include <iostream>

enum XBoxAxis { 
    // Left Thumbstick
    AxisX, // LXAxis,
    AxisY, // LYAxis,
    // Right Thumbstick
    Axis4, // RXAxis,
    Axis5, // RYAxis,
    // Triggers
    RightTrigger, // Axis3
    LeftTrigger // Axis3
};

enum XBoxButtons { 
    A, // Face Buttons
    B,
    X,
    Y,
    LB, // Left Bumper
    RB, // Right Bumper
    Back,
    Start,
    Home, // Guide - The big X
    LeftStick,
    RightStick
};

// enum XBoxHat {
//     Centered = 0,
//     Up = 1,
//     Right = 2,
//     Down = 4,
//     Left = 8,
//     RightUp = Right + Up,
//     RightDown = Right + Down,
//     LeftUp = Left + Up,
//     LeftDown = Left + Down
// };

kglt::ActorID actor_id;
kmVec3 pos = { 0.f, 0.f, -5.f };
kmVec2 rot = { 1.f, 0.5f };
const float sensibility = 50.f;

void joypad_button(kglt::Button button) {
}

void joypad_axis_left(kglt::AxisRange axis_range, kglt::Axis axis) {
    if (axis % 2)
        pos.y += -axis_range / sensibility;
    else
        pos.x += axis_range / sensibility;

    // g_actor->move_to(pos);
}

void joypad_axis_right(kglt::AxisRange axis_range, kglt::Axis axis) {
    if (axis % 2)
        rot.y += -axis_range / sensibility;
    else
        rot.x += axis_range / sensibility;

    // g_actor->rotate_x(rot.y);
    // g_actor->rotate_y(rot.x);
}

int main(int argc, const char *argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window; 
    window = kglt::Window::create();

    window->set_title("KGLT Joypad Sample");

    kglt::Stage& stage = window->scene().stage();
    window->scene().camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        1.0,
        1000.0
    );

    // Thanks to other samples
    auto ui = window->scene().ui_stage();
    ui->set_styles("body { font-family: \"Ubuntu\"; } .thing { font-size: 14; padding-left: 10;};");
    ui->append("<p>").text("Left x-y axis move the capsule.");
    ui->append("<p>").text("Right x-y axis rotate the capsule.");
    ui->append("<p>").text("Button 0 (A) reset the capsule.");
    ui->$("p").add_class("thing");

    ///Shortcut function for loading images
    kglt::TextureID tid = stage.new_texture_from_file("sample_data/sample.tga");
    kglt::MaterialID matid = kglt::create_material_from_texture(stage, tid);

    stage.set_ambient_light(kglt::Colour::WHITE);
    {
        auto light = stage.light(stage.new_light());
        light->set_absolute_position(5.0, 0.0, -5.0);
        light->set_diffuse(kglt::Colour::GREEN);
        light->set_attenuation_from_range(10.0);
    }

    // NOTE: I don't know yet how to manage this kind of pointer-reference system
    actor_id = stage.geom_factory().new_cube(2);
    // actor_id = stage.geom_factory().new_capsule(1,4);

    stage.actor(actor_id)->mesh()->set_material_id(matid);
    stage.actor(actor_id)->set_absolute_position(pos);

    // It would be nice to check if a joypad is connected
    // and the create the reference..
    //
    //if (window->joypad_count() > 0)
    kglt::Joypad& joypad = window->joypad(0);

    // Currently A button on XBOX Controller
    joypad.button_pressed_connect(XBoxButtons::RightStick, [=](kglt::Button button) mutable {
            rot.x = -rot.x;
            rot.y = -rot.y;
    });
    joypad.button_pressed_connect(1, [&](kglt::Button button) mutable {
            /* Reset positions and rotations */
            pos = { 0, 0, -5.f };
            // rot = { 0, 0 };

            stage.actor(actor_id)->set_absolute_rotation(kglt::Degrees(0), 0, 0, pos.z);
            stage.actor(actor_id)->set_absolute_position(pos);
    });

    // Left x-axis
    joypad.axis_while_nonzero_connect(0, joypad_axis_left);
    // Left y-axis
    joypad.axis_while_nonzero_connect(1, joypad_axis_left);
    // Right x-axis
    joypad.axis_while_nonzero_connect(2, joypad_axis_right);
    // Right y-axis
    joypad.axis_while_nonzero_connect(4, [=](kglt::AxisRange range, kglt::Axis) mutable {
            if (range > 0)
                std::cout << (float) range << std::endl;
    });

    // Triggers should work too
    // NOTE: horizontal axis have an even number..
    // Hat experimental
    joypad.hat_changed_connect(0, [=](kglt::HatPosition position, kglt::Hat hat) mutable {
            std::cout << "Hat: " << (int) hat << std::endl;
            std::cout << "Position " << (int) position << std::endl;
            if (position == kglt::HatPosition::Down)
                std::cout << "LeftDown" << std::endl;
    });

    while(window->update()) {
        auto dt = window->delta_time();
        {
            auto actor = stage.actor(actor_id);
            actor->rotate_absolute_x(rot.y*dt*10);
            actor->rotate_absolute_y(rot.x*dt*10);
        }
    }

    stage.delete_actor(actor_id);

    return 0;
}
