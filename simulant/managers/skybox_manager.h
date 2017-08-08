/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
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
    public Managed<Skybox>,
    public generic::Identifiable<SkyID>,
    public StageNode {

public:
    constexpr static float DEFAULT_SIZE = 1024.0f;

    Skybox(SkyID id, SkyManager* manager);

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

    const AABB& aabb() const;

    void update(float step) {}
private:
    friend class SkyManager;

    SkyManager* manager_ = nullptr;

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

typedef generic::TemplatedManager<Skybox, SkyID> TemplatedSkyboxManager;

class SkyManager :
    public TemplatedSkyboxManager,
    public virtual WindowHolder {

public:
    SkyManager(Window* window, Stage* stage);

    SkyID new_skybox_from_folder(const unicode& folder);
    SkyID new_skybox_from_files(
        const unicode& up,
        const unicode& down,
        const unicode& left,
        const unicode& right,
        const unicode& front,
        const unicode& back
    );

    SkyboxPtr skybox(SkyID skybox_id);
    void delete_skybox(SkyID skybox_id);

    Property<SkyManager, Stage> stage = { this, &SkyManager::stage_ };
private:
    Stage* stage_ = nullptr;

};

}
