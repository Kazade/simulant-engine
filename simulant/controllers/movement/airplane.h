#pragma once

#include "../controller.h"
#include "../../event_listener.h"

namespace smlt {
namespace controllers {

class Airplane:
    public Controller,
    public EventListener,
    public Managed<Airplane> {

public:
    Airplane(Controllable* owner, Window* window):
        owner_(owner),
        window_(window) {
    }

    void set_turn_speed(float x) { turn_speed_ = x; }

    void on_key_down(const KeyEvent& evt);
    void on_key_up(const KeyEvent &evt);

    void fixed_update(float step);

    const std::string name() const { return "Airplane"; }

private:
    void on_controller_added(Controllable *controllable);
    void on_controller_removed(Controllable *controllable);

    Controllable* owner_;
    Window* window_;

    bool yaw_left_ = false;
    bool yaw_right_ = false;
    bool pitch_up_ = false;
    bool pitch_down_ = false;
    bool roll_left_ = false;
    bool roll_right_ = false;
    bool accelerating_ = false;
    bool decelerating_ = false;

    float turn_speed_ = 10.0f;
    float throttle_speed_ = 200.0f;
    float throttle_ = 0.0f;
};


}
}
