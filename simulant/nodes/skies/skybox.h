#pragma once

#include <stdexcept>
#include "../stage_node.h"
#include "../../generic/identifiable.h"

namespace smlt {

enum SkyboxFace {
    SKYBOX_FACE_TOP,
    SKYBOX_FACE_BOTTOM,
    SKYBOX_FACE_LEFT,
    SKYBOX_FACE_RIGHT,
    SKYBOX_FACE_FRONT,
    SKYBOX_FACE_BACK,
    SKYBOX_FACE_MAX
};

class SkyboxImageNotFoundError : public std::runtime_error {
public:
    SkyboxImageNotFoundError(const std::string& what):
        std::runtime_error(what) {}
};

class SkyboxImageDuplicateError : public std::runtime_error {
public:
    SkyboxImageDuplicateError(const std::string& what):
        std::runtime_error(what) {}
};

class SkyManager;

class Skybox :
    public generic::Identifiable<SkyID>,
    public ContainerNode {

public:
    constexpr static float DEFAULT_SIZE = 1024.0f;

    Skybox(SkyManager* manager);

    bool init() override;
    void clean_up() override;

    void set_size(float size) { width_ = size; }
    float size() const { return width_; }

    void generate(
        const unicode& up,
        const unicode& down,
        const unicode& left,
        const unicode& right,
        const unicode& front,
        const unicode& back
    );

    void destroy() override;
    void destroy_immediately() override;

    const AABB& aabb() const override;

    void update(float step) override {
        _S_UNUSED(step);
    }

private:
    friend class SkyManager;

    SkyManager* manager_ = nullptr;

    CameraID follow_camera_;

    ActorPtr actor_ = nullptr;
    MeshID mesh_id_;

    MaterialID materials_[SKYBOX_FACE_MAX];

    float width_;
};

}
