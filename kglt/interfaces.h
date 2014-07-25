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
    virtual void update(double step) = 0;
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
    public Boundable {

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

class VertexData;
class IndexData;

class Renderable {
public:
    virtual ~Renderable() {}

    virtual const VertexData& vertex_data() const = 0;
    virtual const IndexData& index_data() const = 0;
    virtual const MeshArrangement arrangement() const = 0;

    virtual void _update_vertex_array_object() = 0;
    virtual void _bind_vertex_array_object() = 0;

    virtual RenderPriority render_priority() const = 0;
    virtual Mat4 final_transformation() const = 0;

    virtual const MaterialID material_id() const = 0;
    virtual const bool is_visible() const = 0;

    virtual MeshID instanced_mesh_id() const = 0;
    virtual SubMeshIndex instanced_submesh_id() const = 0;
};

typedef std::shared_ptr<Renderable> RenderablePtr;


}

#endif // INTERFACES_H
