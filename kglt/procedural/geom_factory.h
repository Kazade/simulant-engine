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

    EntityID new_line(SubSceneID ss, const kmVec3& start, const kmVec3& end);
    EntityID new_sphere(SubSceneID ss, const kmVec3& position, const float diameter);
    EntityID new_cube(SubSceneID ss, const kmVec3& position, const float diameter);
    EntityID new_rectangle_outline(SubSceneID ss, const float width, const float height);
    EntityID new_rectangle(SubSceneID ss, const float width, const float height);
    EntityID new_capsule(SubSceneID, const float diameter=0.5, const float length=1.0);
private:
    Scene& scene_;
};

}

#endif // GEOM_FACTORY_H
