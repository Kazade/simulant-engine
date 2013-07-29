#ifndef BOUNDABLE_H
#define BOUNDABLE_H

#include "kazmath/kazmath.h"
#include "types.h"

namespace kglt {

class Boundable {
public:
    virtual const kmAABB absolute_bounds() const = 0;
    virtual const kmAABB local_bounds() const = 0;
    virtual const Vec3 centre() const = 0;

};

}

#endif // BOUNDABLE_H
