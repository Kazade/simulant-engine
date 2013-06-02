#ifndef GEOM_FACTORY_H
#define GEOM_FACTORY_H

#include "../generic/managed.h"
#include "../types.h"

namespace kglt {

class Scene;

class GeomFactory:
    public Managed<GeomFactory> {

public:
    GeomFactory(Stage& scene);

    EntityID new_line(const kmVec3& start, const kmVec3& end);
    EntityID new_sphere(const kmVec3& position, const float diameter);
    EntityID new_cube(const kmVec3& position, const float diameter);
    EntityID new_rectangle_outline(const float width, const float height);
    EntityID new_rectangle(const float width, const float height);
    EntityID new_capsule(const float diameter=0.5, const float length=1.0);
private:
    Stage& stage_;
};

}

#endif // GEOM_FACTORY_H
