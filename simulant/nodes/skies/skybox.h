#pragma once

#include <stdexcept>
#include "../stage_node.h"
#include "../../generic/identifiable.h"
#include "../../path.h"
#include "../../asset_manager.h"

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

class Skybox:
    public ContainerNode,
    public ChainNameable<Skybox> {

public:
    constexpr static float DEFAULT_SIZE = 128.0f;

    Skybox(Scene* owner);

    void set_size(float size) { width_ = size; }
    float size() const { return width_; }

    void generate(
        const Path& up,
        const Path& down,
        const Path& left,
        const Path& right,
        const Path& front,
        const Path& back,
        const TextureFlags& flags
    );

    bool on_destroy() override;
    bool on_destroy_immediately() override;

    const AABB& aabb() const override;

private:
    friend class SkyManager;

    SkyManager* manager_ = nullptr;

    StageNodeID follow_camera_;

    ActorPtr actor_ = nullptr;
    MeshPtr mesh_;
    MaterialPtr materials_[SKYBOX_FACE_MAX];

    float width_;
};

}
