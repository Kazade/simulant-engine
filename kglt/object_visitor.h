#ifndef OBJECT_VISITOR_H_INCLUDED
#define OBJECT_VISITOR_H_INCLUDED

namespace kglt {

class Mesh;
class Camera;
class Scene;
class Sprite;

class ObjectVisitor {
public:
    virtual ~ObjectVisitor() {}

    virtual void visit(Mesh* mesh) = 0;
    virtual void visit(Camera* camera) = 0;
    virtual void visit(Scene* scene) = 0;
    virtual void visit(Sprite* sprite) = 0;
};

}

#endif // OBJECT_VISITOR_H_INCLUDED
