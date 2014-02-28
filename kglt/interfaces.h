#ifndef INTERFACES_H
#define INTERFACES_H

#include <vector>

namespace kglt {

struct AnimationSequenceStage {
    unicode animation_name;
    float duration;
};

class KeyFrameAnimated {
public:
    virtual void add_sequence(const unicode& name, const std::vector<AnimationSequenceStage>& stages) = 0;
    virtual void play_sequence(const unicode& name) = 0;

    virtual void add_animation(const unicode& name,
        uint32_t start_frame, uint32_t end_frame, float duration
    ) = 0;

    virtual void play_animation(const unicode& name) = 0;
    virtual void queue_next_animation(const unicode& name) = 0;
    virtual void override_playing_animation_duration(const float new_duration) = 0;


    virtual void update(double dt) = 0;
};

}

#endif // INTERFACES_H
