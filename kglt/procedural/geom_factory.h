#ifndef GEOM_FACTORY_H
#define GEOM_FACTORY_H

#include "../generic/managed.h"
#include "../types.h"

namespace kglt {

class Scene;

class GeomFactory:
    public Managed<GeomFactory> {

public:
    GeomFactory(Scene& scene);

    EntityID new_line(StageID ss, const kmVec3& start, const kmVec3& end);
    EntityID new_sphere(StageID ss, const kmVec3& position, const float diameter);
    EntityID new_cube(StageID ss, const kmVec3& position, const float diameter);
    EntityID new_rectangle_outline(StageID ss, const float width, const float height);
    EntityID new_rectangle(StageID ss, const float width, const float height);
    EntityID new_capsule(StageID, const float diameter=0.5, const float length=1.0);
private:
    Scene& scene_;
};

}

#endif // GEOM_FACTORY_H
