#pragma once

#include "../generic/property.h"
#include "../generic/managed.h"
#include "../types.h"
#include "scene.h"
#include "../nodes/ui/image.h"
#include "../nodes/ui/label.h"

namespace smlt {


namespace scenes {

class Splash:
    public Scene<Splash> {

public:
    Splash(Window* window, const std::string& target_scene, float time=4.0):
        Scene<Splash>(window),
        target_(target_scene),
        time_(time) {

    }

private:
    std::string target_;
    float time_;
    float timer_ = 0.0f;

    void activate() override;
    void deactivate() override;

    void load() override;
    void unload() override;

    void update(float dt) override;

    StagePtr stage_;
    CameraPtr camera_;
    PipelinePtr pipeline_;

    ui::Image* text_ = nullptr;
    ui::Image* image_ = nullptr;

    IdleConnectionID connection_;
};

}
}
