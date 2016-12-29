#pragma once

#include <map>

#include "../generic/managed.h"
#include "../generic/identifiable.h"
#include "../nodes/stage_node.h"
#include "../types.h"

#include "./window_holder.h"

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

class SkyboxManager;

class Skybox :
    public Managed<Skybox>,
    public generic::Identifiable<SkyboxID>,
    public StageNode {

public:
    Skybox(SkyboxID id, SkyboxManager* manager);

    bool init() override;
    void cleanup() override;

    void set_width(float width) { width_ = width; }
    const float width() const { return width_; }

    void generate(
        const unicode& up,
        const unicode& down,
        const unicode& left,
        const unicode& right,
        const unicode& front,
        const unicode& back
    );

    void ask_owner_for_destruction() override;

    const AABB aabb() const;
    const AABB transformed_aabb() const;

    void update(double step) {}
private:
    friend class SkyboxManager;

    SkyboxManager* manager_;

    CameraID follow_camera_;

    ActorID actor_id_;
    MeshID mesh_id_;

    MaterialID materials_[SKYBOX_FACE_MAX];

    float width_;
};

typedef Skybox* SkyboxPtr;

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

typedef generic::TemplatedManager<Skybox, SkyboxID> TemplatedSkyboxManager;

class SkyboxManager :
    public TemplatedSkyboxManager,
    public virtual WindowHolder {

public:
    SkyboxManager(WindowBase* window, Stage* stage);

    SkyboxID new_skybox_from_folder(const unicode& folder);
    SkyboxID new_skybox_from_absolute_files(
        const unicode& up,
        const unicode& down,
        const unicode& left,
        const unicode& right,
        const unicode& front,
        const unicode& back
    );

    SkyboxID new_skybox_from_folder_and_relative_files(
        const unicode& folder,
        const unicode& up,
        const unicode& down,
        const unicode& left,
        const unicode& right,
        const unicode& front,
        const unicode& back
    );

    SkyboxID new_skybox_from_folder_and_relative_files(
        const unicode& folder,
        std::map<SkyboxFace, unicode> files
    );

    SkyboxPtr skybox(SkyboxID skybox_id);
    void delete_skybox(SkyboxID skybox_id);

    Property<SkyboxManager, Stage> stage = { this, &SkyboxManager::stage_ };
private:
    Stage* stage_ = nullptr;

};

}
