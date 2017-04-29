#pragma once

#include "../types.h"

namespace smlt {

/**
 * @brief The Locateable class
 *
 * An interface that describes objects that have a position and rotation in space
 */
class Locateable {
public:
    virtual ~Locateable() {}

    virtual smlt::Vec3 position() const = 0;
    virtual smlt::Vec2 position_2d() const = 0;
    virtual smlt::Quaternion rotation() const = 0;
    virtual smlt::Vec3 scale() const = 0;
};

}
