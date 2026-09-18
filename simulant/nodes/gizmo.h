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

#include <vector>

#include "../event_listener.h"
#include "../generic/optional.h"
#include "../math/plane.h"
#include "../math/quaternion.h"
#include "../math/vec2.h"
#include "stage_node.h"

namespace smlt {

class Actor;
class Camera;
class RenderTarget;
class Viewport;

/* A Unity/Blender-style transform gizmo: translate/rotate/scale handles
 * drawn at a node's origin. Typically attached as a mixin to whatever node
 * you want to manipulate, e.g. `target->create_mixin<Gizmo>()` - a mixin
 * shares its base's transform, so the gizmo automatically tracks the
 * target's position/rotation/scale with no extra work.
 *
 * The gizmo renders the handle set for the current mode() and responds to
 * mouse input in two ways:
 *  - If a target Simulant app registers it as an EventListener (the usual
 *    way - it registers/unregisters itself with scene->window in
 *    on_create()/the destructor) and calls set_camera() with the camera
 *    it's rendering through, the gizmo will hit-test/drag itself from the
 *    normal on_mouse_down/move/up callbacks.
 *  - Anything else (e.g. a headless player relaying injected coordinates
 *    over IPC, which never receives real OS mouse events) can drive the
 *    same behaviour directly via begin_drag()/update_drag()/end_drag(),
 *    passing whatever camera/target/viewport it's rendering with.
 *
 * It marks itself and all its handle geometry editor_only(), so it's
 * excluded from normal scene traversal (each_child()/each_descendent()) by
 * virtue of being a mixin, and would be skipped by a future scene
 * serializer too. */
class Gizmo: public StageNode, public EventListener {
public:
    S_DEFINE_STAGE_NODE_META("gizmo");

    enum GizmoMode {
        GIZMO_MODE_TRANSLATE,
        GIZMO_MODE_ROTATE,
        GIZMO_MODE_SCALE
    };

    Gizmo(Scene* owner) :
        StageNode(owner, Meta::node_type) {}
    ~Gizmo();

    bool on_create(Params params) override;
    void on_transformation_changed() override;

    void set_mode(GizmoMode mode);
    GizmoMode mode() const {
        return mode_;
    }

    // The camera the gizmo should hit-test/drag against when driven via
    // the EventListener mouse callbacks (not needed if a caller only ever
    // uses begin_drag()/update_drag()/end_drag() directly).
    void set_camera(CameraPtr camera) {
        camera_ = camera;
    }
    CameraPtr camera() const {
        return camera_;
    }

    bool is_dragging() const {
        return dragging_;
    }

    // Hit-tests the gizmo's current handle set against a screen-space
    // point and, if a handle was hit, starts a drag. Returns true if a
    // drag was started.
    //
    // Takes a non-const Camera* because Camera::unproject_point() (which
    // this needs) isn't const.
    bool begin_drag(Camera* camera, const RenderTarget& target,
                    const Viewport& viewport, const Vec2& screen_point);

    // Updates the in-progress drag (started by begin_drag()) given a new
    // screen-space point. No-op if not currently dragging.
    void update_drag(Camera* camera, const RenderTarget& target,
                     const Viewport& viewport, const Vec2& screen_point);

    // Ends the in-progress drag, if any.
    void end_drag();

private:
    enum class Axis { X, Y, Z };

    // The gizmo is always drawn axis-aligned to *world* space, regardless
    // of the target's own orientation (matching the "Global" mode most
    // editors default to) - otherwise, e.g. dragging the "X" arrow on a
    // node with any non-identity rotation would move it partly off-axis
    // in world space, since the arrow itself would be tilted by the
    // node's rotation. Each handle's position/orientation is therefore
    // recomputed in world space (see reposition_handles()) rather than
    // left to the normal parent-child transform composition, which would
    // otherwise inherit the base's rotation for free (undesirably).
    struct HandleInfo {
        StageNode* node;
        Axis axis;
        float offset; // distance along axis_direction(axis) from the origin
        Quaternion orientation; // desired absolute world-space orientation
    };

    GizmoMode mode_ = GIZMO_MODE_TRANSLATE;

    std::vector<StageNode*> translate_handles_;
    std::vector<StageNode*> rotate_handles_;
    std::vector<StageNode*> scale_handles_;
    std::vector<HandleInfo> handle_infos_;

    CameraPtr camera_ = nullptr;

    bool dragging_ = false;
    Axis drag_axis_ = Axis::X;
    // Both captured once in begin_drag() and reused for the whole drag
    // gesture, rather than recomputed from the target's (moving) transform
    // every update_drag() call - recomputing them against the *current*
    // transform would make the plane/axis follow the very motion the drag
    // is trying to measure, producing runaway feedback (each frame's
    // "delta" gets measured against a plane that already moved to chase
    // the previous frame's delta).
    Vec3 drag_axis_dir_;
    Plane drag_plane_fixed_;
    Vec3 drag_start_hit_point_;
    Vec3 drag_start_position_;
    Quaternion drag_start_orientation_;
    Vec3 drag_start_scale_;

    Vec3 axis_direction(Axis axis) const;
    Quaternion axis_orientation(Axis axis) const;
    Color axis_color(Axis axis) const;
    MaterialPtr axis_material(const Color& color);

    // The gl1x renderer's unlit path renders raw per-vertex colour and
    // never applies the material's base_color (only the lit/software-
    // lighting path modulates vertex colour by base_color) - so for an
    // always-unlit gizmo, the colour has to be baked into the vertex data
    // itself rather than relied on from the material alone.
    void recolor_mesh(const MeshPtr& mesh, const Color& color);

    // Gizmo handles are editor/tooling overlays, not scene content - they
    // shouldn't cast or receive shadows.
    void disable_shadows(Actor* actor);

    void build_translate_handles();
    void build_rotate_handles();
    void build_scale_handles();

    // Repositions/reorients every handle in handle_infos_ using the
    // target's *current* world position but a fixed world-space axis
    // direction/orientation - called once after building the handles, and
    // again on every subsequent on_transformation_changed(), so the gizmo
    // tracks the target's position without inheriting its rotation.
    void reposition_handles();

    Vec3 world_origin() const;

    // Plane containing the axis line, oriented to face the camera as
    // directly as possible - the standard trick for turning a 1D "drag
    // along this axis" gesture into a well-conditioned ray/plane hit.
    Plane drag_plane(Axis axis, Camera* camera) const;

    smlt::optional<Axis> hit_test(Camera* camera, const RenderTarget& target,
                                  const Viewport& viewport,
                                  const Vec2& screen_point) const;

    // Intersects the mouse ray with a fixed plane (established once at
    // begin_drag() and reused for the rest of that drag gesture - see the
    // comment on drag_plane_fixed_ for why it must not be recomputed every
    // frame).
    smlt::optional<Vec3> hit_point_on_plane(const Plane& plane, Camera* camera,
                                            const RenderTarget& target,
                                            const Viewport& viewport,
                                            const Vec2& screen_point) const;

    static float point_segment_distance_2d(const Vec2& p, const Vec2& a,
                                           const Vec2& b);

    void on_mouse_down(const MouseEvent& evt) override;
    void on_mouse_up(const MouseEvent& evt) override;
    void on_mouse_move(const MouseEvent& evt) override;
};

} // namespace smlt
