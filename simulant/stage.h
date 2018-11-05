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

#ifndef STAGE_H
#define STAGE_H

#include <functional>

#include "generic/manual_manager.h"
#include "generic/managed.h"
#include "generic/manager.h"
#include "generic/generic_tree.h"
#include "generic/data_carrier.h"

#include "managers/window_holder.h"
#include "managers/skybox_manager.h"
#include "managers/sprite_manager.h"

#include "nodes/stage_node.h"
#include "nodes/light.h"
#include "types.h"
#include "resource_manager.h"
#include "managers.h"
#include "fog_settings.h"
#include "window.h"

namespace smlt {

namespace ui {
    class UIManager;
}

class Partitioner;

class Debug;
class Sprite;

typedef generic::ManualManager<Actor, ActorID> ActorManager;
typedef generic::ManualManager<Geom, GeomID> GeomManager;
typedef generic::ManualManager<Light, LightID> LightManager;
typedef generic::TemplatedManager<ParticleSystem, ParticleSystemID> ParticleSystemManager;

typedef sig::signal<void (const ActorID&)> ActorCreatedSignal;
typedef sig::signal<void (const ActorID&)> ActorDestroyedSignal;

typedef sig::signal<void (GeomID)> GeomCreatedSignal;
typedef sig::signal<void (GeomID)> GeomDestroyedSignal;

typedef sig::signal<void (ParticleSystemID)> ParticleSystemCreatedSignal;
typedef sig::signal<void (ParticleSystemID)> ParticleSystemDestroyedSignal;

typedef sig::signal<void (CameraID, Viewport)> StagePreRenderSignal;
typedef sig::signal<void (CameraID, Viewport)> StagePostRenderSignal;


extern const Colour DEFAULT_LIGHT_COLOUR;

class Stage:
    public ContainerNode,
    public Managed<Stage>,
    public generic::Identifiable<StageID>,
    public ActorManager,
    public ParticleSystemManager,
    public LightManager,
    public CameraManager,
    public Loadable,    
    public RenderableStage,
    public virtual WindowHolder {

    DEFINE_SIGNAL(ParticleSystemCreatedSignal, signal_particle_system_created);
    DEFINE_SIGNAL(ParticleSystemDestroyedSignal, signal_particle_system_destroyed);

    DEFINE_SIGNAL(StagePreRenderSignal, signal_stage_pre_render);
    DEFINE_SIGNAL(StagePostRenderSignal, signal_stage_post_render);

public:
    Stage(StageID id, Window *parent, AvailablePartitioner partitioner);
    ~Stage();

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
    ActorPtr delete_actor(ActorID e);
    std::size_t actor_count() const { return ActorManager::count(); }

    GeomPtr new_geom_with_mesh(MeshID mid);
    GeomPtr new_geom_with_mesh_at_position(MeshID mid, const Vec3& position, const Quaternion& rotation=Quaternion());
    GeomPtr geom(const GeomID gid) const;
    bool has_geom(GeomID geom_id) const;
    GeomPtr delete_geom(GeomID geom_id);
    std::size_t geom_count() const;

    ParticleSystemPtr new_particle_system();
    ParticleSystemPtr new_particle_system_from_file(const unicode& filename, bool destroy_on_completion=false);
    ParticleSystemPtr new_particle_system_with_parent_from_file(ActorID parent, const unicode& filename, bool destroy_on_completion=false);
    ParticleSystemPtr particle_system(ParticleSystemID pid);
    bool has_particle_system(ParticleSystemID pid) const;
    ParticleSystemPtr delete_particle_system(ParticleSystemID pid);
    std::size_t particle_system_count() const { return ParticleSystemManager::count(); }

    LightPtr new_light_as_directional(const Vec3& direction=Vec3(1, -0.5, 0), const smlt::Colour& colour=DEFAULT_LIGHT_COLOUR);
    LightPtr new_light_as_point(const Vec3& position=Vec3(), const smlt::Colour& colour=DEFAULT_LIGHT_COLOUR);

    LightPtr light(LightID light);
    LightPtr delete_light(LightID light_id);
    std::size_t light_count() const { return LightManager::count(); }

    smlt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const smlt::Colour& c) { ambient_light_ = c; }

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the stage");
    }

    void ask_owner_for_destruction() override;

    Property<Stage, Debug> debug = {this, &Stage::debug_};
    Property<Stage, Partitioner> partitioner = {this, &Stage::partitioner_};
    Property<Stage, ResourceManager> assets = {this, &Stage::resource_manager_};
    Property<Stage, generic::DataCarrier> data = {this, &Stage::data_};
    Property<Stage, ui::UIManager> ui = {this, &Stage::ui_};
    Property<Stage, SkyManager> skies = {this, &Stage::sky_manager_};
    Property<Stage, SpriteManager> sprites = {this, &Stage::sprite_manager_};
    Property<Stage, FogSettings> fog = {this, &Stage::fog_};

    bool init() override;
    void cleanup() override;

    // Updateable interface

    void update(float dt) override;

    // RenderableStage
    void on_render_started() override {}
    void on_render_stopped() override {}

    ActorCreatedSignal& signal_actor_created() { return signal_actor_created_; }
    ActorDestroyedSignal& signal_actor_destroyed() { return signal_actor_destroyed_; }

    GeomCreatedSignal& signal_geom_created() { return signal_geom_created_; }
    GeomDestroyedSignal& signal_geom_destroyed() { return signal_geom_destroyed_; }

    sig::signal<void (LightID)>& signal_light_created() { return signal_light_created_; }
    sig::signal<void (LightID)>& signal_light_destroyed() { return signal_light_destroyed_; }

    const AABB& aabb() const override { return aabb_; }
    const AABB transformed_aabb() const override { return aabb_; }

private:
    AABB aabb_;

    ActorCreatedSignal signal_actor_created_;
    ActorDestroyedSignal signal_actor_destroyed_;

    GeomCreatedSignal signal_geom_created_;
    GeomDestroyedSignal signal_geom_destroyed_;

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
    std::shared_ptr<ResourceManager> resource_manager_;
    smlt::Colour ambient_light_;

    std::unique_ptr<FogSettings> fog_;
    std::unique_ptr<GeomManager> geom_manager_;
    std::unique_ptr<SkyManager> sky_manager_;
    std::unique_ptr<SpriteManager> sprite_manager_;

    generic::DataCarrier data_;

private:
    void on_actor_created(ActorID actor_id);
    void on_actor_destroyed(ActorID actor_id);
};

}
#endif // SUBSCENE_H
