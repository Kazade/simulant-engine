#ifndef STAGE_H
#define STAGE_H

#include <functional>

#include "generic/managed.h"
#include "generic/manager.h"
#include "generic/generic_tree.h"
#include "generic/data_carrier.h"

#include "managers/window_holder.h"
#include "managers/skybox_manager.h"

#include "object.h"
#include "types.h"
#include "resource_manager.h"
#include "window_base.h"

namespace kglt {

class SubActor;
enum ActorChangeType {
    ACTOR_CHANGE_TYPE_SUBACTOR_MATERIAL_CHANGED
};

struct SubActorMaterialChangeData {
    MaterialID old_material_id;
    MaterialID new_material_id;
};

struct ActorChangeEvent {
    ActorChangeType type;
    SubActorMaterialChangeData subactor_material_changed;
};

namespace batcher {
class RenderQueue;
}

class Partitioner;

class Debug;
class Sprite;

typedef generic::TemplatedManager<Actor, ActorID> ActorManager;
typedef generic::TemplatedManager<Geom, GeomID> GeomManager;
typedef generic::TemplatedManager<Light, LightID> LightManager;
typedef generic::TemplatedManager<CameraProxy, CameraID> CameraProxyManager;
typedef generic::TemplatedManager<Sprite, SpriteID> SpriteManager;
typedef generic::TemplatedManager<ParticleSystem, ParticleSystemID> ParticleSystemManager;

typedef sig::signal<void (ActorID)> ActorCreatedSignal;
typedef sig::signal<void (ActorID)> ActorDestroyedSignal;
typedef sig::signal<void (ActorID, ActorChangeEvent)> ActorChangedCallback;

typedef sig::signal<void (GeomID)> GeomCreatedSignal;
typedef sig::signal<void (GeomID)> GeomDestroyedSignal;

typedef sig::signal<void (ParticleSystemID)> ParticleSystemCreatedSignal;
typedef sig::signal<void (ParticleSystemID)> ParticleSystemDestroyedSignal;

class Stage:
    public Managed<Stage>,
    public generic::Identifiable<StageID>,
    public ActorManager,
    public ParticleSystemManager,
    public LightManager,
    public SpriteManager,
    public CameraProxyManager,
    public SkyboxManager,
    public Loadable,
    public SceneNode,
    public Protectable,
    public RenderableStage,
    public generic::DataCarrier,
    public virtual WindowHolder {

    DEFINE_SIGNAL(ParticleSystemCreatedSignal, signal_particle_system_created);
    DEFINE_SIGNAL(ParticleSystemDestroyedSignal, signal_particle_system_destroyed);

public:
    Stage(StageID id, WindowBase *parent, AvailablePartitioner partitioner);
    ~Stage();

    ActorID new_actor(RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorID new_actor_with_mesh(MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);

    ActorID new_actor_with_parent(ActorID parent, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorID new_actor_with_parent_and_mesh(ActorID parent, MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);
    ActorID new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid, RenderableCullingMode mode=RENDERABLE_CULLING_MODE_PARTITIONER);

    GeomID new_geom_with_mesh(MeshID mid);
    GeomID new_geom_with_mesh_at_position(MeshID mid, const Vec3& position, const Quaternion& rotation=Quaternion());
    GeomPtr geom(const GeomID gid) const;
    bool has_geom(GeomID geom_id) const;
    void delete_geom(GeomID geom_id);
    uint32_t geom_count() const;

    ProtectedPtr<Actor> actor(ActorID e);
    const ProtectedPtr<Actor> actor(ActorID e) const;

    bool has_actor(ActorID e) const;
    void delete_actor(ActorID e);
    uint32_t actor_count() const { return ActorManager::manager_count(); }

    ParticleSystemID new_particle_system();
    ParticleSystemID new_particle_system_from_file(const unicode& filename, bool destroy_on_completion=false);
    ParticleSystemID new_particle_system_with_parent_from_file(ActorID parent, const unicode& filename, bool destroy_on_completion=false);
    ParticleSystemPtr particle_system(ParticleSystemID pid);
    bool has_particle_system(ParticleSystemID pid) const;
    void delete_particle_system(ParticleSystemID pid);

    SpriteID new_sprite();
    SpriteID new_sprite_from_file(
        const unicode& filename,
        uint32_t frame_Width, uint32_t frame_height,
        uint32_t margin=0, uint32_t spacing=0,
        std::pair<uint32_t, uint32_t> padding=std::make_pair(0, 0)
    );
    SpritePtr sprite(SpriteID s);
    bool has_sprite(SpriteID s) const;
    void delete_sprite(SpriteID s);
    uint32_t sprite_count() const;

    LightID new_light(LightType type=LIGHT_TYPE_POINT);
    LightID new_light(MoveableObject& parent, LightType type=LIGHT_TYPE_POINT);
    LightPtr light(LightID light);
    void delete_light(LightID light_id);
    uint32_t light_count() const { return LightManager::manager_count(); }

    void host_camera(CameraID c); ///< Create a representation (CameraProxy) of the designated camera
    void evict_camera(CameraID c); ///< Remove the representation of the camera
    ProtectedPtr<CameraProxy> camera(CameraID c);

    kglt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const kglt::Colour& c) { ambient_light_ = c; }

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the stage");
    }

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(this);
        obj._initialize();
    }

    void ask_owner_for_destruction() override;


    Property<Stage, Debug> debug = { this, &Stage::debug_ };
    Property<Stage, batcher::RenderQueue> render_queue = { this, &Stage::render_queue_ };
    Property<Stage, Partitioner> partitioner = { this, &Stage::partitioner_ };
    Property<Stage, ResourceManager> assets = { this, &Stage::resource_manager_ };

    bool init() override;
    void cleanup() override;

    // Updateable interface

    void update(double dt) override;

    // Locateable interface

    Vec3 position() const override { return Vec3(); }
    Vec2 position_2d() const override { return Vec2(); }
    Quaternion rotation() const override { return Quaternion(); }

    // Printable interface
    unicode __unicode__() const override {
        if(has_name()) {
            return name();
        } else {
            return _u("Stage {0}").format(this->id());
        }
    }

    // Nameable interface
    void set_name(const unicode& name) override { name_ = name; }
    const unicode name() const override { return name_; }
    const bool has_name() const override { return !name_.empty(); }

    // RenderableStage
    void on_render_started() override {}
    void on_render_stopped() override {}

    ActorCreatedSignal& signal_actor_created() { return signal_actor_created_; }
    ActorDestroyedSignal& signal_actor_destroyed() { return signal_actor_destroyed_; }
    ActorChangedCallback& signal_actor_changed() { return signal_actor_changed_; }

    GeomCreatedSignal& signal_geom_created() { return signal_geom_created_; }
    GeomDestroyedSignal& signal_geom_destroyed() { return signal_geom_destroyed_; }

    sig::signal<void (LightID)>& signal_light_created() { return signal_light_created_; }
    sig::signal<void (LightID)>& signal_light_destroyed() { return signal_light_destroyed_; }

    sig::signal<void (SpriteID)>& signal_sprite_created() { return signal_sprite_created_; }
    sig::signal<void (SpriteID)>& signal_sprite_destroyed() { return signal_sprite_destroyed_; }

private:
    kglt::Colour ambient_light_;

    ActorCreatedSignal signal_actor_created_;
    ActorDestroyedSignal signal_actor_destroyed_;
    ActorChangedCallback signal_actor_changed_;

    GeomCreatedSignal signal_geom_created_;
    GeomDestroyedSignal signal_geom_destroyed_;

    sig::signal<void (LightID)> signal_light_created_;
    sig::signal<void (LightID)> signal_light_destroyed_;

    sig::signal<void (SpriteID)> signal_sprite_created_;
    sig::signal<void (SpriteID)> signal_sprite_destroyed_;

    std::shared_ptr<Partitioner> partitioner_;

    void set_partitioner(AvailablePartitioner partitioner);

    std::shared_ptr<Debug> debug_;

    CameraID new_camera_proxy(CameraID cam);
    void delete_camera_proxy(CameraID cam);

    unicode name_;

    //FIXME: All managers should be composition rather than inheritence,
    // like this one!

    std::unique_ptr<GeomManager> geom_manager_;
    std::unique_ptr<batcher::RenderQueue> render_queue_;
    std::shared_ptr<ResourceManager> resource_manager_;

private:
    void on_actor_created(ActorID actor_id);
    void on_actor_destroyed(ActorID actor_id);
    void on_subactor_material_changed(ActorID actor_id, SubActor* subactor, MaterialID old, MaterialID newM);
};


}
#endif // SUBSCENE_H
