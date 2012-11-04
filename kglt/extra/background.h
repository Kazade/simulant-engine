#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <string>
#include <vector>
#include <tr1/memory>
#include <cstdint>
#include <kazmath/mat4.h>

#include "../object.h"
#include "../types.h"
#include "../generic/creator.h"

namespace kglt {

class Scene;
class Renderer;

namespace extra {

class Background;

class BackgroundLayer {
public:
    typedef std::tr1::shared_ptr<BackgroundLayer> ptr;

    BackgroundLayer(Background& background, const std::string& image_path);
    ~BackgroundLayer();

    void scroll_x(double amount);
    void scroll_y(double amount);

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    MaterialID material_id() const { return material_id_; }
    EntityID entity() const { return entity_id_; }

    Background& background() { return background_; }

private:
    Background& background_;
    TextureID texture_id_;
    MaterialID material_id_;

    EntityID entity_id_;
    MeshID mesh_id_;

    uint32_t width_;
    uint32_t height_;

    double offset_x_;
    double offset_y_;
};

class Background {
public:
    typedef std::tr1::shared_ptr<Background> ptr;

    Background(Scene& scene, ViewportID viewport);

    void add_layer(const std::string& image_path);
    BackgroundLayer& layer(uint32_t index) { return *layers_.at(index); }
    uint32_t layer_count() const { return layers_.size(); }

    void set_visible_dimensions(double width, double height);

    double visible_x() const { return visible_x_; }
    double visible_y() const { return visible_y_; }

    Scene& scene() { return scene_; }

    SceneGroupID scene_group() const { return background_sg_; }

    static std::tr1::shared_ptr<Background> create(Scene& scene, ViewportID viewport=0) {
        return std::tr1::shared_ptr<Background>(new Background(scene, viewport));
    }

private:
    Scene& scene_;

    ViewportID viewport_;
    SceneGroupID background_sg_;
    CameraID ortho_camera_;

    std::vector<BackgroundLayer::ptr> layers_;

    double visible_x_;
    double visible_y_;

    kmMat4 tmp_projection_;

    void destroy() {}
};

}
}
#endif // BACKGROUND_H
