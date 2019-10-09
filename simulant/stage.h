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

#ifndef STAGE_H
#define STAGE_H

#include <functional>

#include "generic/object_manager.h"
#include "generic/managed.h"
#include "generic/generic_tree.h"
#include "generic/data_carrier.h"
#include "generic/atomic.h"

#include "managers/window_holder.h"
#include "managers/skybox_manager.h"
#include "managers/sprite_manager.h"

#include "nodes/actor.h"
#include "nodes/geom.h"
#include "nodes/particle_system.h"
#include "nodes/stage_node.h"
#include "nodes/light.h"
#include "types.h"
#include "asset_manager.h"
#include "fog_settings.h"

namespace smlt {

namespace ui {
    class UIManager;
}

class Partitioner;

class Debug;
class Sprite;

template<typename T, typename IDType, typename ...Subtypes>
class ManualManager;

typedef ManualManager<Actor, ActorID> ActorManager;
typedef ManualManager<Geom, GeomID> GeomManager;
typedef ManualManager<Light, LightID> LightManager;
typedef ManualManager<ParticleSystem, ParticleSystemID> ParticleSystemManager;
typedef ManualManager<Camera, CameraID> CameraManager;

typedef sig::signal<void (const ActorID&)> ActorCreatedSignal;
typedef sig::signal<void (const ActorID&)> ActorDestroyedSignal;

typedef sig::signal<void (GeomID)> GeomCreatedSignal;
typedef sig::signal<void (GeomID)> GeomDestroyedSignal;

typedef sig::signal<void (ParticleSystemID)> ParticleSystemCreatedSignal;
typedef sig::signal<void (ParticleSystemID)> ParticleSystemDestroyedSignal;

typedef sig::signal<void (CameraID)> CameraCreatedSignal;
typedef sig::signal<void (CameraID)> CameraDestroyedSignal;

typedef sig::signal<void (CameraID, Viewport)> StagePreRenderSignal;
typedef sig::signal<void (CameraID, Viewport)> StagePostRenderSignal;


extern const Colour DEFAULT_LIGHT_COLOUR;

class Stage:
    public TypedDestroyableObject<Stage, Window>,
    public ContainerNode,
    public generic::Identifiable<StageID>,
    public Loadable,    
    public virtual WindowHolder {

    DEFINE_SIGNAL(ParticleSystemCreatedSignal, signal_particle_system_created);
    DEFINE_SIGNAL(ParticleSystemDestroyedSignal, signal_particle_system_destroyed);

    DEFINE_SIGNAL(StagePreRenderSignal, signal_stage_pre_render);
    DEFINE_SIGNAL(StagePostRenderSignal, signal_stage_post_render);

public:
    Stage(StageID id, Window *parent, AvailablePartitioner partitioner);
    virtual ~Stage();

    ActorPtr new_actor(RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorPtr new_actor_with_name(const std::string& name, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);

    ActorPtr new_actor_with_mesh(MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorPtr new_actor_with_name_and_mesh(const std::string& name, MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);

    ActorPtr new_actor_with_parent(ActorID parent, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorPtr new_actor_with_parent_and_mesh(ActorID parent, MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorPtr new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorPtr actor(ActorID e);
    ActorPtr actor(ActorID e) const;
    bool has_actor(ActorID e) const;
    void destroy_actor(ActorID e);
    std::size_t actor_count() const;

    CameraPtr new_camera();
    CameraPtr new_camera_with_orthographic_projection(double left=0, double right=0, double bottom=0, double top=0, double near=-1.0, double far=1.0);
    CameraPtr new_camera_for_ui();
    CameraPtr new_camera_for_viewport(const Viewport& vp);
    CameraPtr camera(CameraID c);
    void destroy_camera(CameraID cid);
    uint32_t camera_count() const;
    bool has_camera(CameraID id) const;
    void destroy_all_cameras();

    GeomPtr new_geom_with_mesh(MeshID mid, const GeomCullerOptions& culler_options=GeomCullerOptions());
    GeomPtr new_geom_with_mesh_at_position(
        MeshID mid, const Vec3& position,
        const Quaternion& rotation=Quaternion(),
        const GeomCullerOptions& culler_options=GeomCullerOptions()
    );
    GeomPtr geom(const GeomID gid) const;
    bool has_geom(GeomID geom_id) const;
    void destroy_geom(GeomID geom_id);
    std::size_t geom_count() const;

    ParticleSystemPtr new_particle_system();
    ParticleSystemPtr new_particle_system_from_file(const unicode& filename, bool destroy_on_completion=false);
    ParticleSystemPtr new_particle_system_with_parent_from_file(ActorID parent, const unicode& filename, bool destroy_on_completion=false);
    ParticleSystemPtr particle_system(ParticleSystemID pid);
    bool has_particle_system(ParticleSystemID pid) const;
    void destroy_particle_system(ParticleSystemID pid);
    std::size_t particle_system_count() const;

    LightPtr new_light_as_directional(const Vec3& direction=Vec3(1, -0.5, 0), const smlt::Colour& colour=DEFAULT_LIGHT_COLOUR);
    LightPtr new_light_as_point(const Vec3& position=Vec3(), const smlt::Colour& colour=DEFAULT_LIGHT_COLOUR);

    LightPtr light(LightID light);
    void destroy_light(LightID light_id);
    std::size_t light_count() const;

    smlt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const smlt::Colour& c) { ambient_light_ = c; }

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the stage");
    }

    Property<Stage, Debug> debug = {this, &Stage::debug_};
    Property<Stage, Partitioner> partitioner = {this, &Stage::partitioner_};
    Property<Stage, AssetManager> assets = {this, &Stage::asset_manager_};
    Property<Stage, generic::DataCarrier> data = {this, &Stage::data_};
    Property<Stage, ui::UIManager> ui = {this, &Stage::ui_};
    Property<Stage, SkyManager> skies = {this, &Stage::sky_manager_};
    Property<Stage, SpriteManager> sprites = {this, &Stage::sprite_manager_};
    Property<Stage, FogSettings> fog = {this, &Stage::fog_};

    bool init() override;
    void clean_up() override;

    // Updateable interface

    void update(float dt) override;

    ActorCreatedSignal& signal_actor_created() { return signal_actor_created_; }
    ActorDestroyedSignal& signal_actor_destroyed() { return signal_actor_destroyed_; }

    GeomCreatedSignal& signal_geom_created() { return signal_geom_created_; }
    GeomDestroyedSignal& signal_geom_destroyed() { return signal_geom_destroyed_; }

    CameraCreatedSignal& signal_camera_created() { return signal_camera_created_; }
    CameraDestroyedSignal& signal_camera_destroyed() { return signal_camera_destroyed_; }

    sig::signal<void (LightID)>& signal_light_created() { return signal_light_created_; }
    sig::signal<void (LightID)>& signal_light_destroyed() { return signal_light_destroyed_; }

    const AABB& aabb() const override { return aabb_; }
    const AABB transformed_aabb() const override { return aabb_; }

    /* Enables the debug actor to allow drawing of debug lines and points */
    Debug* enable_debug(bool v=true);

    /* Implementation for TypedDestroyableObject (INTERNAL) */
    void destroy_object(Actor* object);
    void destroy_object(Light* object);
    void destroy_object(Camera* object);
    void destroy_object(Geom* object);
    void destroy_object(ParticleSystem* object);

    void destroy_object_immediately(Actor* object);
    void destroy_object_immediately(Light* object);
    void destroy_object_immediately(Camera* object);
    void destroy_object_immediately(Geom* object);
    void destroy_object_immediately(ParticleSystem* object);

    bool is_part_of_active_pipeline() const {
        return active_pipeline_count_ > 0;
    }

private:
    AABB aabb_;

    ActorCreatedSignal signal_actor_created_;
    ActorDestroyedSignal signal_actor_destroyed_;

    GeomCreatedSignal signal_geom_created_;
    GeomDestroyedSignal signal_geom_destroyed_;

    CameraCreatedSignal signal_camera_created_;
    CameraDestroyedSignal signal_camera_destroyed_;

    sig::signal<void (LightID)> signal_light_created_;
    sig::signal<void (LightID)> signal_light_destroyed_;

    sig::signal<void (SpriteID)> signal_sprite_created_;
    sig::signal<void (SpriteID)> signal_sprite_destroyed_;

    std::shared_ptr<Partitioner> partitioner_;

    void set_partitioner(AvailablePartitioner partitioner);

    std::shared_ptr<Debug> debug_;
    std::unique_ptr<ui::UIManager> ui_;

    unicode name_;

    //FIXME: All managers should be composition rather than inheritence,
    // like this one!   
    std::shared_ptr<AssetManager> asset_manager_;
    smlt::Colour ambient_light_ = smlt::Colour(0.3, 0.3, 0.3, 1.0);

    std::unique_ptr<FogSettings> fog_;
    std::unique_ptr<GeomManager> geom_manager_;
    std::unique_ptr<SkyManager> sky_manager_;
    std::unique_ptr<SpriteManager> sprite_manager_;

    std::unique_ptr<ActorManager> actor_manager_;
    std::unique_ptr<ParticleSystemManager> particle_system_manager_;
    std::unique_ptr<LightManager> light_manager_;
    std::unique_ptr<CameraManager> camera_manager_;

    generic::DataCarrier data_;

    friend class Pipeline;
    atomic<uint8_t> active_pipeline_count_ = {0};

private:
    void on_actor_created(ActorID actor_id);
    void on_actor_destroyed(ActorID actor_id);

    void clean_up_dead_objects();
    sig::connection clean_up_signal_;
};

}
#endif // SUBSCENE_H
