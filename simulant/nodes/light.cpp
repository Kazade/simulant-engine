//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "light.h"
#include <algorithm>
#include <cmath>

#include "../application.h"
#include "../asset_manager.h"
#include "../assets/material.h"
#include "../math/utils.h"
#include "../meshes/mesh.h"
#include "../renderers/batching/renderable.h"
#include "../scenes/scene.h"

namespace smlt {

namespace {
const int32_t VISUALISATION_CIRCLE_SEGMENTS = 24;
const float DIRECTIONAL_VISUALISATION_RADIUS = 0.5f;
// Point light range commonly defaults to a "gameplay falloff distance"
// scale (e.g. 100 units) that's sensible for lighting calculations but
// would draw a wireframe circle so large it swamps the actual scene
// content and the transform gizmo when the light is selected. Cap the
// *visualization* at a reasonable size - this only affects the
// visualisation drawing, not the light's real range/intensity falloff.
const float MAX_VISUALISATION_RADIUS = 5.0f;

// axis: 0 = circle in the local XY plane, 1 = XZ, 2 = YZ. Builds a closed
// loop of MESH_ARRANGEMENT_LINES segments (rather than a LINE_STRIP) so it
// can share a single submesh/index buffer with the other circles that make
// up the visualisation.
void push_wireframe_circle(VertexData* vdata, IndexData* idata, float radius,
                           int axis, const Color& color) {
    uint32_t start = vdata->count();

    vdata->move_to_end();
    for(int i = 0; i < VISUALISATION_CIRCLE_SEGMENTS; ++i) {
        float t = 2.0f * PI * float(i) / float(VISUALISATION_CIRCLE_SEGMENTS);
        float c = std::cos(t) * radius;
        float s = std::sin(t) * radius;

        Vec3 p;
        switch(axis) {
            case 0:
                p = Vec3(c, s, 0.0f);
                break;
            case 1:
                p = Vec3(c, 0.0f, s);
                break;
            default:
                p = Vec3(0.0f, c, s);
                break;
        }

        vdata->position(p);
        vdata->color(color);
        vdata->move_next();
    }
    vdata->done();

    for(int i = 0; i < VISUALISATION_CIRCLE_SEGMENTS; ++i) {
        idata->index(start + i);
        idata->index(start + (i + 1) % VISUALISATION_CIRCLE_SEGMENTS);
    }
}
} // namespace

Light::Light(Scene* owner, StageNodeType type) :
    ContainerNode(owner, type), type_(LIGHT_TYPE_POINT) {}

void Light::set_type(LightType type) {
    type_ = type;
    visualisation_dirty_ = true;

    /* Don't cull directional lights */
    set_cullable(type_ != LIGHT_TYPE_DIRECTIONAL);
}

bool Light::on_create(Params args) {
    Color c = args.get<FloatArray>("color").value_or(smlt::Color::white());
    set_color(c);

    // Only PointLight declares "range" (it's meaningless for a
    // DirectionalLight), so this is absent for one of the two leaf types -
    // get<float>() returning no_value there is expected, not an error.
    auto range = args.get<float>("range");
    if(range) {
        set_range(range.value());
    }

    auto intensity = args.get<float>("intensity");
    if(intensity) {
        set_intensity(intensity.value());
    }

    return StageNode::on_create(args);
}

void Light::rebuild_visualisation_mesh() {
    if(!visualisation_mesh_) {
        visualisation_mesh_ =
            scene->assets->create_mesh(VertexSpecification::POSITION_AND_DIFFUSE);

        auto mat = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
        mat->set_cull_mode(CULL_MODE_NONE);
        mat->set_depth_write_enabled(false);

        visualisation_submesh_ = visualisation_mesh_->create_submesh(
            "editor_light_visualisation", mat, INDEX_TYPE_16_BIT,
            MESH_ARRANGEMENT_LINES);
    }

    auto& vdata = visualisation_mesh_->vertex_data;
    auto& idata = visualisation_submesh_->index_data;

    vdata->clear();
    idata->clear();

    static const Color visualisation_color(0.9f, 0.85f, 0.3f, 1.0f);

    // Three orthogonal circles give a simple "sphere-ish" wireframe without
    // needing a real wireframe-sphere generator. Radius is the light's
    // actual range for a point light (so it doubles as a range indicator);
    // directional lights have no meaningful range, so use a small fixed
    // size just to mark where the node is.
    float radius = (type_ == LIGHT_TYPE_POINT)
                       ? std::min(range_, MAX_VISUALISATION_RADIUS)
                       : DIRECTIONAL_VISUALISATION_RADIUS;

    for(int axis = 0; axis < 3; ++axis) {
        push_wireframe_circle(vdata.get(), idata.get(), radius, axis,
                              visualisation_color);
    }

    idata->done();

    visualisation_dirty_ = false;
}

void Light::do_generate_renderables(batcher::RenderQueue* render_queue,
                                    const Camera*, const Viewport*,
                                    const DetailLevel, Light**,
                                    const std::size_t,
                                    bool respect_visibility) {
    if(!get_app()->is_editor_mode()) {
        return;
    }

    // Editor-only lights are tooling, not scene content - they shouldn't
    // draw a visualisation for themselves.
    if(is_editor_only()) {
        return;
    }

    if(respect_visibility && !is_visible()) {
        return;
    }

    if(visualisation_dirty_) {
        rebuild_visualisation_mesh();
    }

    Renderable renderable;
    renderable.final_transformation = &transform->world_space_matrix();
    renderable.render_priority = render_priority();
    renderable.is_visible = is_visible();
    renderable.arrangement = visualisation_submesh_->arrangement();
    renderable.vertex_data = visualisation_mesh_->vertex_data.get();
    renderable.index_data = visualisation_submesh_->index_data.get();
    renderable.index_element_count = renderable.index_data->count();
    renderable.material = visualisation_submesh_->material().get();
    renderable.center = transform->position();
    render_queue->insert_renderable(std::move(renderable));
}

} // namespace smlt
