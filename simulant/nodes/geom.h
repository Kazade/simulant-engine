/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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
#include "geoms/geom_culler_opts.h"
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
class Geom:
    public StageNode,
    public virtual Boundable,
    public HasMutableRenderPriority,
    public ChainNameable<Geom> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_GEOM, "geom");
    S_DEFINE_STAGE_NODE_PARAM(Geom, "mesh", MeshPtr, no_value,
                              "The base mesh associated with this geom");
    S_DEFINE_STAGE_NODE_PARAM(Geom, "options", GeomCullerOptions,
                              GeomCullerOptions(),
                              "The options to use when creating the culler");
    S_DEFINE_STAGE_NODE_PARAM(Geom, "position", FloatArray, Vec3(),
                              "The position of the geom");
    S_DEFINE_STAGE_NODE_PARAM(Geom, "orientation", FloatArray, Quaternion(),
                              "The orientation of the geom");
    S_DEFINE_STAGE_NODE_PARAM(Geom, "scale", FloatArray, Vec3(1),
                              "The scale of the geom");

    Geom(Scene* owner);

    const AABB& aabb() const override;

    void do_generate_renderables(batcher::RenderQueue* render_queue,
                                 const Camera*, const Viewport* viewport,
                                 const DetailLevel detail_level, Light** light,
                                 const std::size_t light_count) override;

    bool on_create(Params params) override;

private:
    std::shared_ptr<GeomCuller> culler_;

    AABB aabb_;

public:
    Property<decltype(&Geom::culler_)> culler = {this, &Geom::culler_};
};

} // namespace smlt
