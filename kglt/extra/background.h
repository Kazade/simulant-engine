#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <kazmath/mat4.h>

#include "../generic/managed.h"
#include "../object.h"
#include "../types.h"

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

class Background:
    public Managed<Background> {

public:
    Background(Scene& scene, ViewportID viewport=ViewportID(), BGResizeStyle style = BG_RESIZE_ZOOM);

    void add_layer(const std::string& image_path);
    BackgroundLayer& layer(uint32_t index) { return *layers_.at(index); }
    uint32_t layer_count() const { return layers_.size(); }

    MaterialID material_id() const;

    Stage& stage() { return stage_; }

    bool init() override;
    void cleanup() override;

private:
    Stage& stage_;

    PipelineID pipeline_;
    ViewportID viewport_;
    CameraID ortho_camera_;
    BGResizeStyle style_;

    std::vector<BackgroundLayer::ptr> layers_;

    void destroy() {}

    ActorID actor_;
    MaterialID material_;

    friend class BackgroundLayer;
};

}
}
#endif // BACKGROUND_H
