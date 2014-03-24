#ifndef INTERFACES_H
#define INTERFACES_H

#include <vector>
#include "types.h"

namespace kglt {

struct AnimationSequenceStage {
    unicode animation_name;
    float duration;
};

class KeyFrameAnimated {
public:
    virtual ~KeyFrameAnimated() {}

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


/**
 * @brief The Transformable class
 *
 * An interface that describes objects that can be moved and rotated
 */
class Transformable {
public:
    virtual ~Transformable() {}

    virtual void move_to(const kglt::Vec3& pos) = 0;
    virtual void move_to(const kglt::Vec2& pos) = 0;
    virtual void move_to(float x, float y, float z) = 0;
    virtual void move_to(float x, float y) = 0;

    virtual void rotate_to(const kglt::Degrees& angle) = 0;
    virtual void rotate_to(const kglt::Degrees& angle, float axis_x, float axis_y, float axis_z) = 0;
    virtual void rotate_to(const kglt::Degrees& angle, const kglt::Vec3& axis) = 0;
    virtual void rotate_to(const kglt::Quaternion& rotation) = 0;

    virtual void rotate_x(const kglt::Degrees& angle) = 0;
    virtual void rotate_y(const kglt::Degrees& angle) = 0;
    virtual void rotate_z(const kglt::Degrees& angle) = 0;

    virtual void look_at(const kglt::Vec3& target) = 0;
    virtual void look_at(float x, float y, float z) = 0;
};

/**
 * @brief The Locateable class
 *
 * An interface that describes objects that have a position and rotation in space
 */
class Locateable {
public:
    virtual ~Locateable() {}

    virtual kglt::Vec3 position() const = 0;
    virtual kglt::Vec2 position_2d() const = 0;
    virtual kglt::Quaternion rotation() const = 0;
};

/**
 * @brief The Owned class
 *
 * An interface that describes objects that are owned by a parent
 */
class Ownable {
public:
    virtual ~Ownable() {}
    virtual void ask_owner_for_destruction() = 0;
};


/**
 * @brief The Updateable class
 *
 * Any object that can be updated using a deltatime value
 *
 */
class Updateable {
public:
    virtual ~Updateable() {}
    virtual void update(double dt) = 0;
};


/**
 * @brief The Nameable class
 *
 * Any object that can be given a user-friendly name
 */
class Nameable {
public:
    virtual ~Nameable() {}

    virtual void set_name(const unicode& name) = 0;
    virtual const unicode name() const = 0;
};

}

#endif // INTERFACES_H
