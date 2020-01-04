//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cassert>
#include "animation.h"
#include "logging.h"
#include "macros.h"

namespace smlt {

void KeyFrameAnimated::add_sequence(const std::string& name, const std::vector<AnimationSequenceStage>& stages) {
    _S_UNUSED(name);
    _S_UNUSED(stages);

    assert(0 && "Not implemented");
}

void KeyFrameAnimationState::play_sequence(const std::string& name) {
    _S_UNUSED(name);
    assert(0 && "Not implemented");
}

void KeyFrameAnimated::add_animation(const std::string& name,
    uint32_t start_frame, uint32_t end_frame, float fps
) {
    auto anim = std::make_shared<Animation>();
    anim->frames.first = start_frame;
    anim->frames.second = end_frame;
    anim->duration = (end_frame - start_frame) / fps;
    animations_[name] = anim;

    if(first_animation_.empty()) {
        first_animation_ = name;
    }

    signal_animation_added_(this, name);
}

KeyFrameAnimationState::KeyFrameAnimationState(KeyFrameAnimated* animatable, AnimationUpdatedCallback refresh_animation_state):
    animatable_(animatable),
    refresh_animation_state_(refresh_animation_state) {

    auto anim = animatable_;
    on_animation_added_ = anim->signal_animation_added().connect(
        [this](KeyFrameAnimated* animatable, const std::string& name) {
            if(animatable->animation_count() == 1) {
                play_animation(name);
            }
        }
    );
}

void KeyFrameAnimationState::play_first_animation() {
    play_animation(animatable_->first_animation_);
}

void KeyFrameAnimationState::play_animation(const std::string& name) {
    auto animatable = animatable_;
    if(!animatable) {
        L_WARN("Animatable has been destroyed, not animating");
        current_animation_ = next_animation_ = nullptr;
    }

    auto anim = animatable->animation(name);
    if(!anim) {
        L_WARN("No such animation: " + name);
        return;
    }

    if(current_animation_ == anim) {
        return;
    }

    current_animation_ = anim;
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
    refresh_animation_state_(current_frame_, next_frame_, interp_);
}

void KeyFrameAnimationState::queue_next_animation(const std::string& name) {
    auto animatable = animatable_;
    if(!animatable) {
        L_WARN("Animatable has been destroyed, not animating");
        current_animation_ = next_animation_ = nullptr;
        return;
    }

    auto anim = animatable->animation(name);
    if(!anim) {
        L_WARN("No such animation: " + name);
        return;
    }

    next_animation_ = anim;
}

void KeyFrameAnimationState::override_playing_animation_duration(const float new_duration) {
    current_animation_duration_ = new_duration;
}

void KeyFrameAnimationState::update(float dt) {
    if(!current_animation_){
        return;
    }

    int diff = current_animation_->frames.second - current_animation_->frames.first;
    if(!diff) {
        interp_ = 0.0f;
    } else {
        float divider = current_animation_duration_ / float(diff);
        interp_ += dt / divider;

        if(interp_ >= 1.0f) {
            interp_ = 0.0f;
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

    if(throttle_.update_and_test(dt)) {
        // Frame limit to 60fps max, let's not go crazy
        refresh_animation_state_(current_frame_, next_frame_, interp_);
    }
}


}
