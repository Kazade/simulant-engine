#pragma once

#include "../../asset_manager.h"
#include "../../generic/identifiable.h"
#include "../../path.h"
#include "../stage_node.h"
#include "simulant/nodes/builtins.h"
#include "simulant/utils/params.h"
#include <stdexcept>

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

class SkyboxImageNotFoundError: public std::runtime_error {
public:
    SkyboxImageNotFoundError(const std::string& what) :
        std::runtime_error(what) {}
};

class SkyboxImageDuplicateError: public std::runtime_error {
public:
    SkyboxImageDuplicateError(const std::string& what) :
        std::runtime_error(what) {}
};

class Skybox: public ContainerNode, public ChainNameable<Skybox> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_SKYBOX);
    S_DEFINE_STAGE_NODE_PARAM(Skybox, "source_directory", std::string, no_value,
                              "The directory containing the skybox images");
    S_DEFINE_STAGE_NODE_PARAM(Skybox, "flags", TextureFlags, TextureFlags(),
                              "The flags to apply to the skybox textures");

    constexpr static float DEFAULT_SIZE = 128.0f;

    Skybox(Scene* owner);

    void set_size(float size) {
        width_ = size;
    }
    float size() const {
        return width_;
    }

    void generate(const Path& up, const Path& down, const Path& left,
                  const Path& right, const Path& front, const Path& back,
                  const TextureFlags& flags);

    const AABB& aabb() const override;

private:
    friend class SkyManager;

    bool on_create(Params params) override;

    StageNodeID follow_camera_;

    ActorPtr actor_ = nullptr;
    MeshPtr mesh_;
    MaterialPtr materials_[SKYBOX_FACE_MAX];

    float width_;
};

} // namespace smlt
