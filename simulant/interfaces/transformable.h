#pragma once

#include "transform.h"
#include "../generic/property.h"

namespace smlt {


/**
 * @brief The Transformable class
 *
 * An interface that describes objects that can be moved and rotated
 */
class Transformable {
public:
    virtual ~Transformable() {}

protected:
    Transform transform_;

public:
    S_DEFINE_PROPERTY(transform, &Transformable::transform_);
};

}
