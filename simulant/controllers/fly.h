/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <deque>
#include "./controller.h"
#include "../window_base.h"

#include "../interfaces/transformable.h"

namespace smlt {
namespace controllers {

const int MOUSE_FILTER_BUFFER_SIZE = 10;
const float MOUSE_FILTER_MULTIPLIER = 0.5;

class Fly:
    public Controller,
    public Managed<Fly> {

public:
    Fly(Controllable* container, const Property<smlt::SceneBase, smlt::Window>& window):
        Fly(container, window.get()) {}

    Fly(Controllable* container, Window* window):
        Controller() {

        object_ = dynamic_cast<Transformable*>(container);

        if(!object_) {
            throw std::logic_error("Tried to attach FlyController to something which wasn't an object");
        }

        connections_.push_back(window->keyboard->key_while_pressed_connect(KEYBOARD_CODE_W, [=](KeyboardCode key, float dt) {
            moving_forward_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(KEYBOARD_CODE_S, [=](KeyboardCode key, float dt) {
            moving_backward_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(KEYBOARD_CODE_A, [=](KeyboardCode key, float dt) {
            rotating_left_ = true;
        }));

        connections_.push_back(window->keyboard->key_while_pressed_connect(KEYBOARD_CODE_D, [=](KeyboardCode key, float dt) {
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

    const std::string name() const override { return "Fly by Keyboard"; }

private:
    void late_update(float dt) override {
        if(moving_forward_) {
            object_->move_forward_by(600.0 * dt);
        }

        if(moving_backward_) {
            object_->move_forward_by(-600.0 * dt);
        }

        if(rotating_left_) {
            object_->rotate_global_y_by(Degrees(50.0 * dt));
        }

        if(rotating_right_) {
            object_->rotate_global_y_by(Degrees(-50.0 * dt));
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

        object_->rotate_x_by(Degrees(filtered_y * -150.0f * dt));
        object_->rotate_global_y_by(Degrees(filtered_x * -150.0f * dt));

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

    Transformable* object_ = nullptr;
};

}
}
