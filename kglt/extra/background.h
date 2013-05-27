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

enum BGResizeStyle {
    BG_RESIZE_ZOOM,
    BG_RESIZE_SCALE
};


class Background;

class BackgroundLayer {
public:
    typedef std::shared_ptr<BackgroundLayer> ptr;

    BackgroundLayer(Background& background, const std::string& image_path);
    ~BackgroundLayer();

    void scroll_x(double amount);
    void scroll_y(double amount);

    Background& background() { return background_; }

private:
    Background& background_;
    uint8_t pass_id_;

    TextureID texture_id_;
};

class Background {
public:
    typedef std::shared_ptr<Background> ptr;

    Background(Scene& scene, ViewportID viewport, BGResizeStyle style = BG_RESIZE_ZOOM);
    ~Background();

    void add_layer(const std::string& image_path);
    BackgroundLayer& layer(uint32_t index) { return *layers_.at(index); }
    uint32_t layer_count() const { return layers_.size(); }

    static std::shared_ptr<Background> create(Scene& scene, ViewportID viewport=ViewportID()) {
        return std::shared_ptr<Background>(new Background(scene, viewport));
    }

    MaterialID material_id() const { return material_id_; }

    SubScene& subscene() { return subscene_; }

private:
    SubScene& subscene_;

    ViewportID viewport_;
    CameraID ortho_camera_;
    BGResizeStyle style_;

    std::vector<BackgroundLayer::ptr> layers_;

    void destroy() {}

    Entity* entity_;
    MaterialID material_id_;

    friend class BackgroundLayer;
};

}
}
#endif // BACKGROUND_H
