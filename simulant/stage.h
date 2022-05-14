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
#include "threads/atomic.h"

#include "managers/window_holder.h"

#include "nodes/skies/skybox_manager.h"
#include "nodes/sprites/sprite_manager.h"
#include "nodes/actor.h"
#include "nodes/geom.h"
#include "nodes/particle_system.h"
#include "nodes/stage_node.h"
#include "nodes/light.h"
#include "nodes/mesh_instancer.h"
#include "nodes/stage_node_manager.h"

#include "types.h"
#include "asset_manager.h"

#include "macros.h"
#include "stage_manager.h"

namespace smlt {

namespace ui {
    class UIManager;
}

class Partitioner;

class Debug;
class Sprite;

typedef StageNodeManager<StageNodePool, ActorID, Actor> ActorManager;
typedef StageNodeManager<StageNodePool, GeomID, Geom> GeomManager;
typedef StageNodeManager<StageNodePool, LightID, Light> LightManager;
typedef StageNodeManager<StageNodePool, ParticleSystemID, ParticleSystem> ParticleSystemManager;
typedef StageNodeManager<StageNodePool, CameraID, Camera> CameraManager;
typedef StageNodeManager<StageNodePool, MeshInstancerID, MeshInstancer> MeshInstancerManager;

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

typedef sig::signal<void (MeshInstancerID)> MeshInstancerCreatedSignal;
typedef sig::signal<void (MeshInstancerID)> MeshInstancerDestroyedSignal;

extern const Colour DEFAULT_LIGHT_COLOUR;

class Stage:
    public TypedDestroyableObject<Stage, StageManager>,
    public ContainerNode,
    public generic::Identifiable<StageID>,
    public Loadable,
    public ChainNameable<Stage>,
    public RefCounted<Stage> {

    DEFINE_SIGNAL(MeshInstancerCreatedSignal, signal_mesh_instancer_created);
    DEFINE_SIGNAL(MeshInstancerDestroyedSignal, signal_mesh_instancer_destroyed);

    DEFINE_SIGNAL(ParticleSystemCreatedSignal, signal_particle_system_created);
    DEFINE_SIGNAL(ParticleSystemDestroyedSignal, signal_particle_system_destroyed);

    DEFINE_SIGNAL(StagePreRenderSignal, signal_stage_pre_render);
    DEFINE_SIGNAL(StagePostRenderSignal, signal_stage_post_render);

    /* Necessary to access asset_manager before the assets Property<> is initialized */
    friend class ui::UIManager;

public:
    Stage(
        StageManager *parent,
        StageNodePool* node_pool,
        AvailablePartitioner partitioner
    );

    virtual ~Stage();

    ActorPtr new_actor();
    ActorPtr new_actor_with_name(const std::string& name);

    ActorPtr new_actor_with_mesh(MeshID mid);
    ActorPtr new_actor_with_name_and_mesh(const std::string& name, MeshID mid);

    ActorPtr new_actor_with_parent(ActorID parent);
    ActorPtr new_actor_with_parent_and_mesh(ActorID parent, MeshID mid);
    ActorPtr new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid);
    ActorPtr actor(ActorID e);
    ActorPtr actor(ActorID e) const;
    bool has_actor(ActorID e) const;
    void destroy_actor(ActorID e);
    std::size_t actor_count() const;

    /**
     * @brief Creates a new MeshInstancer from a mesh
     * and returns it
     * @param mid The ID of the mesh this instancer will be able to spawn
     * @return a pointer to the new mesh instancer, or a null pointer on failure
     */
    MeshInstancerPtr new_mesh_instancer(MeshID mid);

    /**
     * @brief Destroys a MeshInstancer by its ID.
     * @param The ID of the MeshInstancer to destroy.
     * @return true on success, false if the MeshInstancerID was invalid.
     */
    bool destroy_mesh_instancer(MeshInstancerID mid);

    /**
     * @brief Returns the MeshInstancerPtr associated with the ID
     * @param mid - the id of the MeshInstancer to retrieve
     * @return a valid MeshInstancerPtr if the ID was valid, or null
     */
    MeshInstancerPtr mesh_instancer(MeshInstancerID mid);

    /**
     * @brief Returns the number of MeshInstancers in the stage
     * @return the number of MeshInstancers in the stage
     */
    std::size_t mesh_instancer_count() const;

    /**
     * @brief Checks to see if this MeshInstancer exists
     * @param mid
     * @return true if it exists, false otherwise
     */
    bool has_mesh_instancer(MeshInstancerID mid) const;

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
        const Vec3& scale=Vec3(1, 1, 1),
        const GeomCullerOptions& culler_options=GeomCullerOptions()
    );
    GeomPtr geom(const GeomID gid) const;
    bool has_geom(GeomID geom_id) const;
    void destroy_geom(GeomID geom_id);
    std::size_t geom_count() const;

    ParticleSystemPtr new_particle_system(ParticleScriptID particle_script);
    ParticleSystemPtr new_particle_system_with_parent(ParticleScriptID particle_script, ActorID parent);
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
        _S_UNUSED(x);
        _S_UNUSED(y);
        _S_UNUSED(z);

        throw std::logic_error("You cannot move the stage");
    }

    bool init() override;
    void clean_up() override;

    // Updateable interface

    void update(float dt) override;
    void late_update(float dt) override;

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
    void destroy_object(MeshInstancer* object);

    void destroy_object_immediately(Actor* object);
    void destroy_object_immediately(Light* object);
    void destroy_object_immediately(Camera* object);
    void destroy_object_immediately(Geom* object);
    void destroy_object_immediately(ParticleSystem* object);
    void destroy_object_immediately(MeshInstancer* object);

    bool is_part_of_active_pipeline() const {
        return active_pipeline_count_ > 0;
    }

private:
    StageNodePool* node_pool_ = nullptr;

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

    //FIXME: All managers should be composition rather than inheritence,
    // like this one!
    std::shared_ptr<AssetManager> asset_manager_;
    std::unique_ptr<ui::UIManager> ui_;

    unicode name_;

    smlt::Colour ambient_light_ = smlt::Colour(0.3, 0.3, 0.3, 1.0);

    std::unique_ptr<GeomManager> geom_manager_;
    std::unique_ptr<SkyManager> sky_manager_;
    std::unique_ptr<SpriteManager> sprite_manager_;
    std::unique_ptr<MeshInstancerManager> mesh_instancer_manager_;
    std::unique_ptr<ActorManager> actor_manager_;
    std::unique_ptr<ParticleSystemManager> particle_system_manager_;
    std::unique_ptr<LightManager> light_manager_;
    std::unique_ptr<CameraManager> camera_manager_;

    generic::DataCarrier data_;

    friend class Pipeline;
    thread::Atomic<uint8_t> active_pipeline_count_ = {0};

private:
    void on_actor_created(ActorID actor_id);
    void on_actor_destroyed(ActorID actor_id);

    void clean_up_dead_objects();

public:
    Property<decltype(&Stage::debug_)> debug = {this, &Stage::debug_};
    Property<decltype(&Stage::partitioner_)> partitioner = {this, &Stage::partitioner_};
    Property<decltype(&Stage::asset_manager_)> assets = {this, &Stage::asset_manager_};
    Property<decltype(&Stage::data_)> data = {this, &Stage::data_};
    Property<decltype(&Stage::ui_)> ui = {this, &Stage::ui_};
    Property<decltype(&Stage::sky_manager_)> skies = {this, &Stage::sky_manager_};
    Property<decltype(&Stage::sprite_manager_)> sprites = {this, &Stage::sprite_manager_};
};

}
#endif // SUBSCENE_H
