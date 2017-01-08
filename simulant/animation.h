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

#include <unordered_map>
#include <memory>
#include <vector>
#include "deps/kazsignal/kazsignal.h"

namespace smlt {

struct AnimationSequenceStage {
    std::string animation_name;
    float duration;
};

class KeyFrameAnimated;

typedef sig::signal<void (KeyFrameAnimated*, const std::string&)> SignalAnimationAdded;

class KeyFrameAnimated {
    DEFINE_SIGNAL(SignalAnimationAdded, signal_animation_added);
public:
    virtual ~KeyFrameAnimated() {}

    void add_sequence(const std::string& name, const std::vector<AnimationSequenceStage>& stages);
    void add_animation(const std::string& name, uint32_t start_frame, uint32_t end_frame, float fps);

    bool has_animations() const { return !animations_.empty(); }
    uint32_t animation_count() const { return animations_.size(); }
protected:
    //Animation stuff
    friend class KeyFrameAnimationState;

    struct Animation {
        Animation():
            duration(0) {}

        Animation(double duration, uint32_t start, uint32_t end):
            duration(duration),
            frames(std::make_pair(start, end)) {}

        double duration;
        std::pair<uint32_t, uint32_t> frames;
    };

    typedef std::unordered_map<std::string, std::shared_ptr<Animation> > AnimationMap;
    AnimationMap animations_;
    std::string first_animation_;

    Animation* animation(const std::string& name) {
        auto it = animations_.find(name);
        if(it == animations_.end()) return nullptr;

        return (*it).second.get();
    }
};

// args: current_frame, next_frame, interp
typedef std::function<void (int32_t, int32_t, float)> AnimationUpdatedCallback;

class KeyFrameAnimationState {
public:
    KeyFrameAnimationState(
        std::weak_ptr<KeyFrameAnimated> animatable,
        AnimationUpdatedCallback refresh_animation_state
    );

    ~KeyFrameAnimationState() {
        on_animation_added_.disconnect();
    }

    void play_first_animation();
    void play_animation(const std::string& name);
    void queue_next_animation(const std::string& name);
    void play_sequence(const std::string& name);

    void override_playing_animation_duration(const float new_duration);
    void update(double dt);

    uint32_t current_frame() const { return current_frame_; }
    uint32_t next_frame() const { return next_frame_; }
    float interp() const { return interp_; }
private:
    std::weak_ptr<KeyFrameAnimated> animatable_;

    KeyFrameAnimated::Animation* current_animation_ = nullptr;
    KeyFrameAnimated::Animation* next_animation_ = nullptr;
    float current_animation_duration_ = 0.0;

    uint32_t current_frame_ = 0;
    uint32_t next_frame_ = 0;
    double interp_ = 0.0;

    AnimationUpdatedCallback refresh_animation_state_;
    sig::connection on_animation_added_;
};


}
