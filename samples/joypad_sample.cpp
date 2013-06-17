#include <kglt/kglt.h>
#include <kglt/shortcuts.h>

kglt::Actor* g_actor;
kmVec3 pos = { 0.f, 0.f, -5.f };
kmVec2 rot = { 0.f, 0.f };
const float sensibility = 50.f;

void joypad_button(kglt::Button button) {
    /* Reset positions and rotations */
    pos = { 0, 0, -5.f };
    rot = { 0, 0 };

    g_actor->rotate_to(0, 0, 0, pos.z);
}

void joypad_axis_left(kglt::AxisRange axis_range, kglt::Axis axis) {
    if (axis % 2)
        pos.y += -axis_range / sensibility;
    else
        pos.x += axis_range / sensibility;

    g_actor->move_to(pos);
}

void joypad_axis_right(kglt::AxisRange axis_range, kglt::Axis axis) {
    if (axis % 2)
        rot.y += -axis_range / sensibility;
    else
        rot.x += axis_range / sensibility;

    g_actor->rotate_x(rot.y);
    g_actor->rotate_y(rot.x);
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

    // It would be nice to check if a joypad is connected
    // and the create the reference..
    //
    //if (window->joypad_count() > 0)
    kglt::Joypad& joypad = window->joypad(0);

    // Currently A button on XBOX Controller
    joypad.button_pressed_connect(0, joypad_button);
    // Left x-axis
    joypad.axis_while_nonzero_connect(0, joypad_axis_left);
    // Left y-axis
    joypad.axis_while_nonzero_connect(1, joypad_axis_left);
    // Right x-axis
    joypad.axis_while_nonzero_connect(2, joypad_axis_right);
    // Right y-axis
    joypad.axis_while_nonzero_connect(3, joypad_axis_right);
    // Triggers should work too
    // NOTE: horizontal axis have an even number..


    // Thanks to other samples
    auto ui = window->scene().ui_stage();
    ui->set_styles("body { font-family: \"Ubuntu\"; } .thing { font-size: 14; padding-left: 10;};");
    ui->append("<p>").text("Left x-y axis move the capsule.");
    ui->append("<p>").text("Right x-y axis rotate the capsule.");
    ui->append("<p>").text("Button 0 (A) reset the capsule.");
    ui->$("p").add_class("thing");

    ///Shortcut function for loading images
    kglt::TextureID tid = kglt::create_texture_from_file(stage, "sample_data/sample.tga");
    kglt::MaterialID matid = kglt::create_material_from_texture(stage, tid);

    stage.set_ambient_light(kglt::Colour::white);
    kglt::Light& light = stage.light(stage.new_light());
    light.move_to(5.0, 0.0, -5.0);
    light.set_diffuse(kglt::Colour::green);
    light.set_attenuation_from_range(10.0);

    // NOTE: I don't know yet how to manage this kind of pointer-reference system
    g_actor = &stage.actor(stage.geom_factory().new_capsule());
    g_actor->mesh().lock()->set_material_id(matid);

    g_actor->move_to(pos);

    while(window->update()) {

    }

    return 0;
}
