#pragma once

#include <stdexcept>
#include "../stage_node.h"
#include "../../generic/identifiable.h"
#include "../../path.h"
#include "../../asset_manager.h"
#include "../../path.h"

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


struct SkyboxParams {
    Path source_directory;

    SkyboxParams(const Path& directory):
        source_directory(directory) {}
};

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

    const AABB& aabb() const override;

private:
    friend class SkyManager;

    StageNodeID follow_camera_;

    ActorPtr actor_ = nullptr;
    MeshPtr mesh_;
    MaterialPtr materials_[SKYBOX_FACE_MAX];

    float width_;
};


template<>
struct stage_node_traits<Skybox> {
    const static StageNodeType node_type = STAGE_NODE_TYPE_SKYBOX;
    typedef SkyboxParams params_type;
};


}
