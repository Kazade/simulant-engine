#include "../../scene.h"
#include "../../camera.h"
#include "../../pipeline.h"

#include "interface.h"

namespace kglt {
namespace extra {
namespace ui {

SubScene& Interface::subscene() { return scene_.subscene(subscene_); }

Interface::Interface(Scene &scene, uint32_t width_in_pixels, uint32_t height_in_pixels):
    scene_(scene),
    width_(width_in_pixels),
    height_(height_in_pixels) {

    subscene_ = scene.new_subscene(PARTITIONER_NULL); //Don't cull the UI
    camera_ = subscene().new_camera();

    subscene().camera(camera_).set_orthographic_projection(0, width_, 0, height_, -1.0, 1.0);
    scene_.pipeline().add_stage(subscene_, camera_, ViewportID(), TextureID(), kglt::RENDER_PRIORITY_FOREGROUND);
}



}
}
}
