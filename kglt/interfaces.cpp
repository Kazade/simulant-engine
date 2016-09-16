#include "interfaces.h"

namespace kglt {

std::ostream& operator<< (std::ostream& o, Printable const& instance) {
    return o << instance.__unicode__().encode();
}

void KeyFrameAnimated::add_sequence(const unicode& name, const std::vector<AnimationSequenceStage>& stages) {
    assert(0 && "Not implemented");
}

void KeyFrameAnimated::play_sequence(const unicode& name) {
    assert(0 && "Not implemented");
}

void KeyFrameAnimated::add_animation(const unicode& name,
    uint32_t start_frame, uint32_t end_frame, float duration
) {
    auto& anim = animations_[name];
    anim.frames.first = start_frame;
    anim.frames.second = end_frame;
    anim.duration = duration;

    if(animations_.size() == 1) {
        current_animation_ = &anim;
        current_animation_duration_ = current_animation_->duration;
        current_frame_ = current_animation_->frames.first;
        next_frame_ = current_frame_ + 1;
        if(next_frame_ > current_animation_->frames.second) {
            next_frame_ = current_frame_;
        }
        interp_ = 0.0;

        refresh_animation_state();
    }
}

void KeyFrameAnimated::play_animation(const unicode& name) {
    auto it = animations_.find(name);
    if(it == animations_.end()) {
        L_WARN("No such animation: " + name.encode());
        return;
    }

    if(current_animation_ == &(*it).second) {
        return;
    }

    current_animation_ = &(*it).second;
    current_animation_duration_ = current_animation_->duration;
    next_animation_ = nullptr; //Wipe out the next animation, otherwise we'll get unexpected behaviour

    //Set the current frame and next frame appropriately
    current_frame_ = current_animation_->frames.first;
    next_frame_ = current_frame_ + 1;
    if(next_frame_ > current_animation_->frames.second) {
        //Handle the case where the animation is just a single frame
        next_frame_ = current_frame_;
    }
    interp_ = 0.0;
    refresh_animation_state();
}

void KeyFrameAnimated::queue_next_animation(const unicode& name) {
    auto it = animations_.find(name);
    if(it == animations_.end()) {
        L_WARN("No such animation: " + name.encode());
        return;
    }

    next_animation_ = &(*it).second;
}

void KeyFrameAnimated::override_playing_animation_duration(const float new_duration) {
    current_animation_duration_ = new_duration;
}

void KeyFrameAnimated::update(double dt) {
    if(animations_.empty()){
        return;
    }

    int diff = current_animation_->frames.second - current_animation_->frames.first;
    if(!diff) {
        interp_ = 0.0f;
    } else {
        float divider = current_animation_duration_ / double(diff);
        interp_ += dt / divider;

        if(interp_ >= 1.0) {
            interp_ = 0.0;
            current_frame_ = next_frame_;
            next_frame_++;

            //FIXME: Add and handle loop=false
            if(next_frame_ > current_animation_->frames.second) {
                if(next_animation_) {
                    current_animation_ = next_animation_;
                    current_animation_duration_ = current_animation_->duration;
                    next_animation_ = nullptr;
                }
                next_frame_ = current_animation_->frames.first;
            }
        }
    }

    refresh_animation_state();
}

}
