#pragma once

#include <deque>
#include "./controller.h"
#include "../window_base.h"
#include "../object.h"

namespace kglt {
namespace controllers {

const int MOUSE_FILTER_BUFFER_SIZE = 10;
const float MOUSE_FILTER_MULTIPLIER = 0.5;

class Fly:
    public Controller,
    public Managed<Fly> {

public:
    Fly(Controllable* container, const Property<kglt::ScreenBase, kglt::WindowBase>& window):
        Fly(container, window.get()) {}

    Fly(Controllable* container, WindowBase* window):
        Controller("fly") {

        object_ = dynamic_cast<MoveableObject*>(container);

        if(!object_) {
            throw std::logic_error("Tried to attach FlyController to something which wasn't an object");
        }

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_W, [=](SDL_Keysym key, double dt) {
            moving_forward_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_S, [=](SDL_Keysym key, double dt) {
            moving_backward_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_A, [=](SDL_Keysym key, double dt) {
            rotating_left_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(SDL_SCANCODE_D, [=](SDL_Keysym key, double dt) {
            rotating_right_ = true;
        }));

        connections_.push_back(window->mouse().motion_event_connect([=](uint32_t x, uint32_t y, uint32_t relx, uint32_t rely) {
            mouse_x_buffer_.push_back(relx);
            mouse_y_buffer_.push_back(rely);

            if(mouse_x_buffer_.size() > MOUSE_FILTER_BUFFER_SIZE) {
                mouse_x_buffer_.pop_front();
            }

            if(mouse_y_buffer_.size() > MOUSE_FILTER_BUFFER_SIZE) {
                mouse_y_buffer_.pop_front();
            }
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
            object_->rotate_global_y(Degrees(50.0 * dt));
        }

        if(rotating_right_) {
            object_->rotate_global_y(Degrees(-50.0 * dt));
        }

        // http://www.flipcode.com/archives/Smooth_Mouse_Filtering.shtml
        float filtered_x = 0, filtered_y = 0;
        float multiplier = 1.0;
        for(uint32_t i = 0; i < MOUSE_FILTER_BUFFER_SIZE; ++i) {
            float this_x = 0, this_y = 0;
            if(i < mouse_x_buffer_.size()) {
                this_x = mouse_x_buffer_[i];
            }
            if(i < mouse_y_buffer_.size()) {
                this_y = mouse_y_buffer_[i];
            }

            filtered_x += (this_x * multiplier);
            filtered_y += (this_y * multiplier);
            multiplier *= MOUSE_FILTER_MULTIPLIER;
        }

        filtered_x /= MOUSE_FILTER_BUFFER_SIZE;
        filtered_y /= MOUSE_FILTER_BUFFER_SIZE;

        object_->rotate_x(Degrees(filtered_y * -150.0f * dt));
        object_->rotate_global_y(Degrees(filtered_x * -150.0f * dt));

        moving_forward_ = moving_backward_ = false;
        rotating_left_ = rotating_right_ = false;
    }

    bool moving_forward_ = false;
    bool moving_backward_ = false;
    bool rotating_left_ = false;
    bool rotating_right_ = false;

    std::deque<int32_t> mouse_x_buffer_;
    std::deque<int32_t> mouse_y_buffer_;

    std::vector<InputConnection> connections_;

    MoveableObject* object_ = nullptr;
};

}
}
