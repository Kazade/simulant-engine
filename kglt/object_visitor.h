#ifndef OBJECT_VISITOR_H_INCLUDED
#define OBJECT_VISITOR_H_INCLUDED

namespace kglt {

class Object;
class Mesh;
class Camera;
class Scene;
class Sprite;
class Background;
class BackgroundLayer;
class UI;
class Text;

namespace ui {
    class Element;
}

class ObjectVisitor {
public:
    virtual ~ObjectVisitor() {}

    virtual void visit(Mesh* mesh) = 0;
    virtual void visit(Camera* camera) = 0;
    virtual void visit(Scene* scene) = 0;
    virtual void visit(Sprite* sprite) = 0;
    virtual void visit(Background* background) = 0;
    virtual void visit(BackgroundLayer* layer) = 0;
    virtual void visit(UI* ui) = 0;
    virtual void visit(ui::Element* element) = 0;
    virtual void visit(Text* text) = 0;

    virtual bool pre_visit(Object* obj) = 0; ///< Return false to cancel visiting a node
    virtual void post_visit(Object* obj) = 0;
};

}

#endif // OBJECT_VISITOR_H_INCLUDED
