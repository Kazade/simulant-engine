#pragma once

#include "./controller.h"
#include "../window_base.h"
#include "../object.h"

namespace kglt {
namespace controllers {

class Fly : public Controller {
public:
    Fly(Controllable* container, const Property<kglt::ScreenBase, kglt::WindowBase>& window):
        Fly(container, window.get()) {}

    Fly(Controllable* container, WindowBase* window):
        Controller("fly") {

        object_ = dynamic_cast<Object*>(container);

        if(!object_) {
            throw LogicError("Tried to attach FlyController to something which wasn't an object");
        }

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_UP, [=](SDL_Keysym key, double dt) {
            moving_forward_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_DOWN, [=](SDL_Keysym key, double dt) {
            moving_backward_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_LEFT, [=](SDL_Keysym key, double dt) {
            rotating_left_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_RIGHT, [=](SDL_Keysym key, double dt) {
            rotating_right_ = true;
        }));

    }

    ~Fly() {
        for(auto& conn: connections_) {
            conn.disconnect();
        }
    }

private:
    void do_post_update(double dt) override {
        if(moving_forward_) {
            object_->move_forward(600.0 * dt);
        }

        if(moving_backward_) {
            object_->move_forward(-600.0 * dt);
        }

        if(rotating_left_) {
            object_->rotate_y(Degrees(50.0 * dt));
        }

        if(rotating_right_) {
            object_->rotate_y(Degrees(-50.0 * dt));
        }

        moving_forward_ = moving_backward_ = false;
        rotating_left_ = rotating_right_ = false;
    }

    bool moving_forward_ = false;
    bool moving_backward_ = false;
    bool rotating_left_ = false;
    bool rotating_right_ = false;

    std::vector<InputConnection> connections_;

    Object* object_ = nullptr;
};

}
}
