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

namespace new_batcher {
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

class Stage:
    public Managed<Stage>,
    public generic::Identifiable<StageID>,
    public ResourceManager,
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

public:
    Stage(StageID id, WindowBase *parent, AvailablePartitioner partitioner);

    ActorID new_actor();
    ActorID new_actor(MeshID mid); //FIXME: Deprecate
    ActorID new_actor(MeshID mid, bool make_responsive, bool make_collidable); //FIXME: deprecate

    ActorID new_actor_with_mesh(MeshID mid) { return new_actor(mid); }

    ActorID new_actor_with_parent(ActorID parent);
    ActorID new_actor_with_parent(ActorID parent, MeshID mid); //FIXME: deprecate
    ActorID new_actor_with_parent_and_mesh(ActorID parent, MeshID mid);
    ActorID new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid);

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
    ProtectedPtr<Sprite> sprite(SpriteID s);
    bool has_sprite(SpriteID s) const;
    void delete_sprite(SpriteID s);
    uint32_t sprite_count() const;

    LightID new_light(LightType type=LIGHT_TYPE_POINT);
    LightID new_light(MoveableObject& parent, LightType type=LIGHT_TYPE_POINT);
    ProtectedPtr<Light> light(LightID light);
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

    void ask_owner_for_destruction();

    /*
     *  ResourceManager interface follows
     *  Pass-thrus to the parent
     */

    //Mesh functions
    virtual MeshID new_mesh(bool garbage_collect=true) override {
        return window->new_mesh(garbage_collect);
    }

    virtual MeshID new_mesh_from_file(const unicode& path, bool garbage_collect=true) override {
        return window->new_mesh_from_file(path, garbage_collect);
    }

    MeshID new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, bool garbage_collect=true) override {
        return window->new_mesh_from_tmx_file(tmx_file, layer_name, tile_render_size, garbage_collect);
    }

    MeshID new_mesh_from_heightmap(
        const unicode& image_file, float spacing=1.0, float min_height=-64,
        float max_height=64.0, const HeightmapDiffuseGenerator& generator=HeightmapDiffuseGenerator(), bool garbage_collect=true) override {
        return window->new_mesh_from_heightmap(image_file, spacing, min_height, max_height, generator, garbage_collect);
    }

    virtual MeshID new_mesh_as_cube(float width, bool garbage_collect=true) {
        return window->new_mesh_as_cube(width, garbage_collect);
    }

    virtual MeshID new_mesh_as_cylinder(float diameter, float length, int segments=20, int stacks=20, bool garbage_collect=true) {
        return window->new_mesh_as_cylinder(diameter, length, segments, stacks, garbage_collect);
    }

    virtual MeshID new_mesh_as_box(float width, float height, float depth, bool garbage_collect=true) {
        return window->new_mesh_as_box(width, height, depth, garbage_collect);
    }

    virtual MeshID new_mesh_as_sphere(float diameter, bool garbage_collect=true) {
        return window->new_mesh_as_sphere(diameter, garbage_collect);
    }

    MeshID new_mesh_as_rectangle(float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), bool garbage_collect=true) override {
        return window->new_mesh_as_rectangle(width, height, offset, material, garbage_collect);
    }

    MeshID new_mesh_as_capsule(float diameter, float length, int segments=20, int stacks=20, bool garbage_collect=true) {
        return window->new_mesh_as_capsule(diameter, length, segments, stacks, garbage_collect);
    }

    MeshID new_mesh_from_vertices(const std::vector<Vec2> &vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool garbage_collect=true) override {
        return window->new_mesh_from_vertices(vertices, arrangement, garbage_collect);
    }

    MeshID new_mesh_from_vertices(const std::vector<Vec3> &vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool garbage_collect=true) override {
        return window->new_mesh_from_vertices(vertices, arrangement, garbage_collect);
    }

    MeshID new_mesh_with_alias(const unicode& alias, bool garbage_collect=true) override {
        return window->new_mesh_with_alias(alias, garbage_collect);
    }

    MeshID new_mesh_with_alias_from_file(const unicode& alias, const unicode &path, bool garbage_collect=true) override {
        return window->new_mesh_with_alias_from_file(alias, path, garbage_collect);
    }

    MeshID new_mesh_with_alias_as_cube(const unicode& alias, float width, bool garbage_collect=true) override {
        return window->new_mesh_with_alias_as_cube(alias, width, garbage_collect);
    }

    MeshID new_mesh_with_alias_as_sphere(const unicode& alias, float diameter, bool garbage_collect=true) override {
        return window->new_mesh_with_alias_as_sphere(alias, diameter, garbage_collect);
    }

    MeshID new_mesh_with_alias_as_rectangle(const unicode &alias, float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), bool garbage_collect=true) override {
        return window->new_mesh_with_alias_as_rectangle(alias, width, height, offset, material, garbage_collect);
    }

    virtual MeshID new_mesh_with_alias_as_cylinder(const unicode& alias, float diameter, float length, int segments=20, int stacks=20, bool garbage_collect=true) {
        return window->new_mesh_with_alias_as_cylinder(alias, diameter, length, segments, stacks, garbage_collect);
    }

    MeshID get_mesh_with_alias(const unicode& alias) override {
        return window->get_mesh_with_alias(alias);
    }

    void delete_mesh(MeshID m) {
        window->delete_mesh(m);
    }

    MaterialID new_material_with_alias(const unicode& alias, bool garbage_collect=true) override {
        return window->new_material_with_alias(alias, garbage_collect);
    }

    MaterialID new_material_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect=true) override {
        return window->new_material_with_alias_from_file(alias, path, garbage_collect);
    }

    MaterialID new_material_from_texture(TextureID texture, bool garbage_collect=true) override {
        return window->new_material_from_texture(texture, garbage_collect);
    }

    MaterialID get_material_with_alias(const unicode& alias) override {
        return window->get_material_with_alias(alias);
    }

    void delete_material(MaterialID m) {
        window->delete_material(m);
    }

    virtual ProtectedPtr<Mesh> mesh(MeshID m) { return window->mesh(m); }
    virtual const ProtectedPtr<Mesh> mesh(MeshID m) const { return window->mesh(m); }

    virtual bool has_mesh(MeshID m) const { return window->has_mesh(m); }
    virtual uint32_t mesh_count() const { return window->mesh_count(); }


    //Texture functions
    virtual TextureID new_texture(bool garbage_collect=true) override { return window->new_texture(garbage_collect); }
    virtual TextureID new_texture_from_file(const unicode& path, TextureFlags flags=0, bool garbage_collect=true) {
        return window->new_texture_from_file(path, flags, garbage_collect);
    }
    virtual TextureID new_texture_with_alias(const unicode& alias, bool garbage_collect=true) override {
        return window->new_texture_with_alias(alias, garbage_collect);
    }
    virtual TextureID new_texture_with_alias_from_file(const unicode& alias, const unicode& path, TextureFlags flags=0, bool garbage_collect=true) override {
        return window->new_texture_with_alias_from_file(alias, path, flags, garbage_collect);
    }
    virtual TextureID get_texture_with_alias(const unicode& alias) override {
        return window->get_texture_with_alias(alias);
    }
    virtual void delete_texture(TextureID t) override {
        window->delete_texture(t);
    }

    virtual TexturePtr texture(TextureID t) { return window->texture(t); }
    virtual const TexturePtr texture(TextureID t) const { return window->texture(t); }

    virtual bool has_texture(TextureID t) const { return window->has_texture(t); }
    virtual uint32_t texture_count() const { return window->texture_count(); }
    virtual void mark_texture_as_uncollected(TextureID t) { window->mark_texture_as_uncollected(t); }

    //Sound functions
    virtual SoundID new_sound(bool garbage_collect=true) override { return window->new_sound(garbage_collect); }
    virtual SoundID new_sound_from_file(const unicode& path, bool garbage_collect=true) override {
        return window->new_sound_from_file(path, garbage_collect);
    }

    virtual SoundID new_sound_with_alias(const unicode& alias, bool garbage_collect=true) override {
        return window->new_sound_with_alias(alias, garbage_collect);
    }

    virtual SoundID new_sound_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect=true) override {
        return window->new_sound_with_alias_from_file(alias, path, garbage_collect);
    }

    virtual SoundID get_sound_with_alias(const unicode& alias) {
        return window->get_sound_with_alias(alias);
    }

    virtual void delete_sound(SoundID t) { window->delete_sound(t); }

    virtual ProtectedPtr<Sound> sound(SoundID s) { return window->sound(s); }
    virtual const ProtectedPtr<Sound> sound(SoundID s) const { return window->sound(s); }

    virtual bool has_sound(SoundID s) const { return window->has_sound(s); }
    virtual uint32_t sound_count() const { return window->sound_count(); }


    //Material functions
    virtual MaterialID new_material(bool garbage_collect=true) { return window->new_material(garbage_collect); }
    virtual MaterialID new_material_from_file(const unicode& path, bool garbage_collect=true) {
        return window->new_material_from_file(path, garbage_collect);
    }

    virtual MaterialPtr material(MaterialID m) { return window->material(m); }
    virtual const MaterialPtr material(MaterialID m) const { return window->material(m); }

    virtual bool has_material(MaterialID m) const { return window->has_material(m); }
    virtual uint32_t material_count() const { return window->material_count(); }
    virtual void mark_material_as_uncollected(MaterialID t) { window->mark_material_as_uncollected(t); }

    Property<Stage, Debug> debug = { this, &Stage::debug_ };
    Property<Stage, new_batcher::RenderQueue> render_queue = { this, &Stage::render_queue_ };
    Property<Stage, Partitioner> partitioner = { this, &Stage::partitioner_ };

    bool init();
    void cleanup() override;

    // Updateable interface

    void update(double dt);

    // Locateable interface

    Vec3 position() const override { return Vec3(); }
    Vec2 position_2d() const override { return Vec2(); }
    Quaternion rotation() const override { return Quaternion(); }

    // Printable interface
    unicode __unicode__() const {
        if(has_name()) {
            return name();
        } else {
            return _u("Stage {0}").format(this->id());
        }
    }

    // Nameable interface
    void set_name(const unicode& name) { name_ = name; }
    const unicode name() const { return name_; }
    const bool has_name() const { return !name_.empty(); }


    // Default stuff
    MaterialID default_material_id() const { return window->default_material_id(); }
    TextureID default_texture_id() const { return window->default_texture_id(); }
    MaterialID clone_default_material(bool garbage_collect=true) { return window->clone_default_material(garbage_collect); }
    unicode default_material_filename() const { return window->default_material_filename(); }

    // RenderableStage
    void on_render_started() {}
    void on_render_stopped() {}

    typedef sig::signal<void (ActorID)> ActorCreatedSignal;
    typedef sig::signal<void (ActorID)> ActorDestroyedSignal;
    typedef sig::signal<void (ActorID, ActorChangeEvent)> ActorChangedCallback;

    typedef sig::signal<void (GeomID)> GeomCreatedSignal;
    typedef sig::signal<void (GeomID)> GeomDestroyedSignal;

    ActorCreatedSignal& signal_actor_created() { return signal_actor_created_; }
    ActorDestroyedSignal& signal_actor_destroyed() { return signal_actor_destroyed_; }
    ActorChangedCallback& signal_actor_changed() { return signal_actor_changed_; }

    GeomCreatedSignal& signal_geom_created() { return signal_geom_created_; }
    GeomDestroyedSignal& signal_geom_destroyed() { return signal_geom_destroyed_; }

    sig::signal<void (ParticleSystemID)>& signal_particle_system_created() { return signal_particle_system_created_; }
    sig::signal<void (ParticleSystemID)>& signal_particle_system_destroyed() { return signal_particle_system_destroyed_; }

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

    sig::signal<void (ParticleSystemID)> signal_particle_system_created_;
    sig::signal<void (ParticleSystemID)> signal_particle_system_destroyed_;

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
    std::unique_ptr<new_batcher::RenderQueue> render_queue_;

private:
    void on_actor_created(ActorID actor_id);
    void on_actor_destroyed(ActorID actor_id);
    void on_subactor_material_changed(ActorID actor_id, SubActor* subactor, MaterialID old, MaterialID newM);
};


}
#endif // SUBSCENE_H
