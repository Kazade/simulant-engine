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

#include "../generic/managed.h"
#include "../generic/manual_object.h"
#include "../interfaces.h"
#include "../meshes/mesh.h"
#include "../sound.h"

#include "stage_node.h"

namespace smlt {


class GeomCuller;


enum GeomCullerType {
    GEOM_CULLER_TYPE_OCTREE,
    GEOM_CULLER_TYPE_QUADTREE
};

struct GeomCullerOptions {
    GeomCullerType type = GEOM_CULLER_TYPE_OCTREE;
    uint8_t octree_max_depth = 4;
    uint8_t quadtree_max_depth = 4;
};

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
    public TypedDestroyableObject<Geom, Stage>,
    public StageNode,
    public virtual Boundable,
    public AudioSource,
    public HasMutableRenderPriority,
    public ChainNameable<Geom>  {

public:
    Geom(
        Stage* stage,
        SoundDriver *sound_driver,
        MeshPtr mesh,
        const Vec3& position=Vec3(),
        const Quaternion rotation=Quaternion(),
        const Vec3& scale=Vec3(1, 1, 1),
        GeomCullerOptions culler_options=GeomCullerOptions()
    );

    const AABB& aabb() const override;

    void clean_up() override {
        StageNode::clean_up();
    }

    bool init() override;

    void _get_renderables(batcher::RenderQueue* render_queue, const CameraPtr camera, const DetailLevel detail_level) override;

private:
    MeshPtr mesh_;
    GeomCullerOptions culler_options_;
    Vec3 desired_transform;
    Quaternion desired_rotation;
    Vec3 desired_scale;

    std::shared_ptr<GeomCuller> culler_;

    AABB aabb_;

public:
    Property<decltype(&Geom::culler_)> culler = {this, &Geom::culler_};

};

}

