/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>

#include "../generic/managed.h"
#include "../generic/identifiable.h"
#include "../nodes/stage_node.h"
#include "../types.h"
#include "./window_holder.h"

namespace smlt {

class Stage;

enum SkyboxFace {
    SKYBOX_FACE_TOP,
    SKYBOX_FACE_BOTTOM,
    SKYBOX_FACE_LEFT,
    SKYBOX_FACE_RIGHT,
    SKYBOX_FACE_FRONT,
    SKYBOX_FACE_BACK,
    SKYBOX_FACE_MAX
};

class SkyManager;

class Skybox :
    public generic::Identifiable<SkyID>,
    public ContainerNode {

public:
    constexpr static float DEFAULT_SIZE = 1024.0f;

    Skybox(SkyID id, SkyManager* manager);

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

template<typename T, typename IDType, typename ...Subtypes>
class ManualManager;

typedef ManualManager<Skybox, SkyID> TemplatedSkyboxManager;

class SkyManager :
    public virtual WindowHolder {

    friend class Skybox;

public:
    SkyManager(Window* window, Stage* stage);

    SkyManager(const SkyManager& rhs) = delete;
    SkyManager& operator=(const SkyManager&) = delete;

    SkyboxPtr new_skybox_from_folder(const unicode& folder);
    SkyboxPtr new_skybox_from_files(
        const unicode& up,
        const unicode& down,
        const unicode& left,
        const unicode& right,
        const unicode& front,
        const unicode& back
    );

    SkyboxPtr skybox(SkyID skybox_id);
    void destroy_skybox(SkyID skybox_id);

    Property<Stage* SkyManager::*> stage = { this, &SkyManager::stage_ };
private:
    Stage* stage_ = nullptr;

    std::shared_ptr<TemplatedSkyboxManager> sky_manager_;

};

}
