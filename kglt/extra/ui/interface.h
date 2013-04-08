#ifndef INTERFACE_H
#define INTERFACE_H

#include "../../generic/managed.h"
#include"../../types.h"

#include <Rocket/Core/SystemInterface.h>

namespace kglt {
namespace extra {
namespace ui {

class Interface;

class Interface :
    public Managed<Interface>,
    public Rocket::Core::SystemInterface {

public:
    Interface(Scene& scene, uint32_t width_in_pixels, uint32_t height_in_pixels);

    Scene& scene() { return scene_; }
    SubScene& subscene();

    uint16_t width_in_pixels() const { return width_; }
    uint16_t height_in_pixels() const { return height_; }

private:    
    Scene& scene_;
    SubSceneID subscene_;
    CameraID camera_;

    uint32_t width_;
    uint32_t height_;
};

}
}
}

#endif // INTERFACE_H
