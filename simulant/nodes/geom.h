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

#include "../generic/managed.h"
#include "../interfaces.h"
#include "../meshes/mesh.h"
#include "../sound.h"

#include "stage_node.h"

namespace smlt {


class GeomCuller;

/**
 * @brief The Geom class
 *
 * A Geom is a fixed piece of geometry, like Actors they are
 * constructed from a mesh, but unlike actors they are completely
 * immovable during their lifetime. This gives partitioners
 * the freedom to split the geometry as necessary for improved performance
 * or even store entirely cached versions of the geometry.
 *
 * Also unlike an actor, a mesh is a requirement.
 */
class Geom :
    public StageNode,
    public virtual Boundable,
    public Managed<Geom>,
    public generic::Identifiable<GeomID>,
    public Source {

public:
    Geom(GeomID id, Stage* stage, SoundDriver *sound_driver, MeshID mesh, const Vec3& position=Vec3(), const Quaternion rotation=Quaternion());

    const AABB& aabb() const;

    void ask_owner_for_destruction();

    RenderPriority render_priority() const { return render_priority_; }

    void cleanup() override {
        StageNode::cleanup();
    }

    Property<Geom, GeomCuller> culler = {this, &Geom::culler_};

    bool init() override;

    std::vector<std::shared_ptr<Renderable>> _get_renderables(const Frustum& frustum, DetailLevel detail_level) const;
private:
    MeshID mesh_id_;
    RenderPriority render_priority_ = RENDER_PRIORITY_MAIN;

    std::shared_ptr<GeomCuller> culler_;

    AABB aabb_;

    void update(float dt) {
        update_source(dt);
    }
};

}

