#pragma once

#include "../types.h"

namespace smlt {

/**
 * @brief The Boundable class
 *
 * Any object that can have a bounding box
 */
class Boundable {
public:
    virtual const AABB& aabb() const = 0;

    virtual float width() const {
        AABB box = aabb();
        return box.width();
    }

    virtual float height() const {
        AABB box = aabb();
        return box.height();
    }

    virtual float depth() const {
        AABB box = aabb();
        return box.depth();
    }

    virtual float half_width() const { return width() * 0.5f; }
    virtual float half_height() const { return height() * 0.5f; }
    virtual float half_depth() const { return depth() * 0.5f; }

    virtual float diameter() const { return std::max(width(), std::max(height(), depth())); }
    virtual float radius() const { return diameter() * 0.5f; }
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
        return box.centre();
    }
};

}

