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

    virtual void rotate_around(const kglt::Vec3& axis, const kglt::Degrees& degrees) = 0;

    void rotate_global_x(const kglt::Degrees& degrees) {
        const static Vec3 X(1, 0, 0);
        rotate_around(X, degrees);
    }

    void rotate_global_y(const kglt::Degrees& degrees) {
        const static Vec3 Y(0, 1, 0);
        rotate_around(Y, degrees);
    }

    void rotate_global_z(const kglt::Degrees& degrees) {
        const static Vec3 Z(0, 0, 1);
        rotate_around(Z, degrees);
    }

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
    virtual void update(double step) = 0;
    virtual void pre_update(double dt) {}
    virtual void post_update(double dt) {}

    virtual void pre_fixed_update(double step) {}
    virtual void fixed_update(double step) {}
    virtual void post_fixed_update(double step) {}
};

class Printable {
public:
    virtual unicode __unicode__() const = 0;

    friend std::ostream& operator<< (std::ostream& o, Printable const& instance);
};

std::ostream& operator<< (std::ostream& o, Printable const& instance);


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
    virtual const bool has_name() const = 0;
};


/**
 * @brief The Boundable class
 *
 * Any object that can have a bounding box
 */
class Boundable {
public:
    virtual const AABB aabb() const = 0;

    virtual const float width() const {
        AABB box = aabb();
        return box.max.x - box.min.x;
    }

    virtual const float height() const {
        AABB box = aabb();
        return box.max.y - box.min.y;
    }

    virtual const float depth() const {
        AABB box = aabb();
        return box.max.z - box.min.z;
    }

    virtual const float half_width() const { return width() * 0.5f; }
    virtual const float half_height() const { return height() * 0.5f; }
    virtual const float half_depth() const { return depth() * 0.5f; }

    virtual const float diameter() const { return std::max(width(), std::max(height(), depth())); }
    virtual const float radius() const { return diameter() * 0.5f; }
};

/**
 * @brief The BoundableEntity class
 *
 * Any object that can be contained within a bounding box, but
 * can be positioned somewhere other than 0,0,0
 */



class BoundableEntity:
    public virtual Boundable {

public:
    virtual const AABB transformed_aabb() const = 0;
    virtual const Vec3 centre() const {
        AABB box = transformed_aabb();
        return Vec3(
            box.min.x + half_width(),
            box.min.y + half_height(),
            box.min.z + half_depth()
        );
    }
};


class RenderableStage {
    /*
     *  Keeps track of the number of pipelines a stage of some kind is
     *  active on
     */
public:
    void increment_render_count() {
        render_count_++;
        if(render_count_ == 1) {
            on_render_started();
        }
    }

    void decrement_render_count() {
        render_count_--;
        assert(render_count_ >= 0);

        if(render_count_ == 0) {
            on_render_stopped();
        }
    }

    bool is_being_rendered() const { return bool(render_count_); }

private:
    int render_count_ = 0;

    virtual void on_render_started() = 0;
    virtual void on_render_stopped() = 0;
};


class RenderTarget {
public:
    virtual ~RenderTarget() {}

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    virtual void set_clear_every_frame(uint32_t clear_flags=BUFFER_CLEAR_ALL, const kglt::Colour& colour=kglt::Colour::BLACK) {
        clear_flags_ = clear_flags;
        clear_colour_ = colour;
    }
    virtual uint32_t clear_every_frame_flags() const { return clear_flags_; }
    virtual kglt::Colour clear_every_frame_colour() const { return clear_colour_; }

private:
    uint32_t clear_flags_ = BUFFER_CLEAR_ALL;
    kglt::Colour clear_colour_ = kglt::Colour::BLACK;
};

}

#endif // INTERFACES_H
