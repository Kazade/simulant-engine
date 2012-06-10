#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <string>
#include <vector>
#include <tr1/memory>
#include <cstdint>

#include "object.h"
#include "types.h"
#include "object_visitor.h"

namespace kglt {

class Scene;
class Background;
class Renderer;

class BackgroundLayer : Object {
public:
    BackgroundLayer(Background& background, const std::string& image_path);
    ~BackgroundLayer();

    void scroll_x(double amount);
    void scroll_y(double amount);

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    TextureID texture_id() const { return texture_id_; }

    Background& background() { return background_; }

    void accept(ObjectVisitor& visitor) {
        visitor.pre_visit(this);

        for(Object* child: children_) {
            child->accept(visitor);
        }

        if(is_visible()) {
            visitor.visit(this);
        }
        visitor.post_visit(this);
    }
private:
    Background& background_;
    TextureID texture_id_;
    MeshID mesh_id_;

    uint32_t width_;
    uint32_t height_;

    double offset_x_;
    double offset_y_;
};

class Background : public Object {
public:
    Background();

    void add_layer(const std::string& image_path);
    BackgroundLayer& layer(uint32_t index) { return *layers_.at(index); }
    uint32_t layer_count() const { return layers_.size(); }

    void set_visible_dimensions(double width, double height);

    void pre_visit(ObjectVisitor& visitor);
    void post_visit(ObjectVisitor &visitor);

    void accept(ObjectVisitor& visitor) {
        visitor.pre_visit(this);

        for(Object* child: children_) {
            child->accept(visitor);
        }

        if(is_visible()) {
            visitor.visit(this);
        }
        visitor.post_visit(this);
    }
private:
    std::vector<std::tr1::shared_ptr<BackgroundLayer> > layers_;
    double visible_x_;
    double visible_y_;

};

}
#endif // BACKGROUND_H
