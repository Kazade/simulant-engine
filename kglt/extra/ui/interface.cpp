#include <Rocket/Core.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>

#include "../../scene.h"
#include "../../camera.h"
#include "../../pipeline.h"

#include "interface.h"

namespace kglt {
namespace extra {
namespace ui {

SubScene& Interface::subscene() { return scene_.subscene(subscene_); }

class RocketSystemInterface : public Rocket::Core::SystemInterface {
public:
    RocketSystemInterface() {

    }

    virtual float GetElapsedTime() {
        logging::warn("Not implemented", __FILE__, __LINE__);
        return 0;
    }
};

class RocketRenderInterface : public Rocket::Core::RenderInterface {
public:
    RocketRenderInterface() {

    }

    void RenderGeometry(
        Rocket::Core::Vertex* vertices,
        int num_vertices,
        int* indices,
        int num_indices,
        Rocket::Core::TextureHandle texture,
        const Rocket::Core::Vector2f& translation) {

        logging::warn("Not implemented", __FILE__, __LINE__);
    }

    void EnableScissorRegion(bool enable) {
        logging::warn("Not implemented", __FILE__, __LINE__);
    }

    void SetScissorRegion(int x, int y, int width, int height) {
        logging::warn("Not implemented", __FILE__, __LINE__);
    }
};


static std::tr1::shared_ptr<RocketSystemInterface> rocket_system_interface_;
static std::tr1::shared_ptr<RocketRenderInterface> rocket_render_interface_;

Interface::Interface(Scene &scene, uint32_t width_in_pixels, uint32_t height_in_pixels):
    scene_(scene),
    width_(width_in_pixels),
    height_(height_in_pixels) {

    if(!rocket_system_interface_) {
        rocket_system_interface_.reset(new RocketSystemInterface());
        Rocket::Core::SetSystemInterface(rocket_system_interface_.get());

        rocket_render_interface_.reset(new RocketRenderInterface());
        Rocket::Core::SetRenderInterface(rocket_render_interface_.get());

        Rocket::Core::Initialise();
    }

    //FIXME: Change name for each interface
    context_ = Rocket::Core::CreateContext("default", Rocket::Core::Vector2i(width_in_pixels, height_in_pixels));

    subscene_ = scene.new_subscene(PARTITIONER_NULL); //Don't cull the UI
    camera_ = subscene().new_camera();

    subscene().camera(camera_).set_orthographic_projection(0, width_, 0, height_, -1.0, 1.0);
    scene_.pipeline().add_stage(subscene_, camera_, ViewportID(), TextureID(), kglt::RENDER_PRIORITY_FOREGROUND);
}



}
}
}
