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

    virtual const float width() const {
        AABB box = aabb();
        return box.width();
    }

    virtual const float height() const {
        AABB box = aabb();
        return box.height();
    }

    virtual const float depth() const {
        AABB box = aabb();
        return box.depth();
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
        return box.centre();
    }
};

}

