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

    ActorID new_line(const kmVec3& start, const kmVec3& end);
    ActorID new_sphere(const kmVec3& position, const float diameter);
    ActorID new_cube(const kmVec3& position, const float diameter);
    ActorID new_rectangle_outline(const float width, const float height);
    ActorID new_rectangle(const float width, const float height);
    ActorID new_capsule(const float diameter=0.5, const float length=1.0);
private:
    Stage& stage_;
};

}

#endif // GEOM_FACTORY_H
