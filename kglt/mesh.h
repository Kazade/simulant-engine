#ifndef NEWMESH_H
#define NEWMESH_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <set>
#include <memory>

#include "deps/kazmath/kazmath.h"

#include "generic/managed.h"
#include "generic/identifiable.h"
#include "generic/property.h"

#include "loadable.h"
#include "resource.h"
#include "vertex_data.h"
#include "types.h"
#include "interfaces.h"
#include "animation.h"

namespace kglt {

#ifdef KGLT_GL_VERSION_2X
/* FIXME: REMOVE! */
class VertexArrayObject;
class BufferObject;
typedef std::shared_ptr<VertexArrayObject> VertexArrayObjectPtr;
typedef std::shared_ptr<BufferObject> BufferObjectPtr;
#endif

class ResourceManager;

class MeshInterface:
    public virtual Boundable {

public:
    virtual ~MeshInterface() {}

    Property<MeshInterface, VertexData> shared_data = { this, &MeshInterface::get_shared_data };

private:
    virtual VertexData* get_shared_data() const = 0;
};

class SubMeshInterface:
    public Boundable {

public:
    virtual ~SubMeshInterface() {}

    virtual const MaterialID material_id() const = 0;
    virtual const MeshArrangement arrangement() const = 0;

#ifdef KGLT_GL_VERSION_2X
    virtual void _update_vertex_array_object() = 0;
    virtual void _bind_vertex_array_object() = 0;
#endif

    Property<SubMeshInterface, VertexData> vertex_data = { this, &SubMeshInterface::get_vertex_data };
    Property<SubMeshInterface, IndexData> index_data = { this, &SubMeshInterface::get_index_data };

private:
    virtual VertexData* get_vertex_data() const = 0;
    virtual IndexData* get_index_data() const = 0;
};

enum VertexSharingMode {
    VERTEX_SHARING_MODE_SHARED,
    VERTEX_SHARING_MODE_INDEPENDENT
};

class SubMesh :
    public SubMeshInterface,
    public Managed<SubMesh>,
    public std::enable_shared_from_this<SubMesh> {

public:
    SubMesh(
        Mesh* parent,
        const std::string& name,
        MaterialID material,
        MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES,
        VertexSharingMode vertex_sharing = VERTEX_SHARING_MODE_SHARED,
        VertexSpecification vertex_specification = VertexSpecification()
    );

    virtual ~SubMesh();

    const MaterialID material_id() const;
    void set_material_id(MaterialID mat);

    void set_diffuse(const kglt::Colour& colour);

    const MeshArrangement arrangement() const { return arrangement_; }

    const AABB aabb() const {
        return bounds_;
    }

    bool uses_shared_vertices() const { return uses_shared_data_; }
    void reverse_winding();
    void transform_vertices(const Mat4 &transformation);
    void set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass=0);

    void _recalc_bounds();

#ifdef KGLT_GL_VERSION_2X
    void _update_vertex_array_object();
    void _bind_vertex_array_object();
#endif

    void generate_texture_coordinates_cube(uint32_t texture=0);

    VertexData* get_vertex_data() const;
    IndexData* get_index_data() const;

    const std::string& name() const { return name_; }

public:
    typedef sig::signal<void (SubMesh*, MaterialID, MaterialID)> MaterialChangedCallback;

    MaterialChangedCallback& signal_material_changed() {
        return signal_material_changed_;
    }

private:
    friend class Mesh;

    sig::connection material_change_connection_;

    Mesh* parent_;
    std::string name_;

    MaterialPtr material_;
    MeshArrangement arrangement_;
    bool uses_shared_data_;

    VertexData* vertex_data_ = nullptr;
    IndexData* index_data_ = nullptr;

#ifdef KGLT_GL_VERSION_2X
    VertexArrayObjectPtr vertex_array_object_;
#endif

    bool vertex_data_dirty_ = false;
    bool index_data_dirty_ = false;

    AABB bounds_;

    sig::connection vrecalc_;
    sig::connection irecalc_;

    MaterialChangedCallback signal_material_changed_;
};


enum MeshAnimationType {
    MESH_ANIMATION_TYPE_NONE,
    MESH_ANIMATION_TYPE_VERTEX_MORPH
};


typedef sig::signal<void (Mesh*, MeshAnimationType, uint32_t)> SignalAnimationEnabled;

class Mesh :
    public MeshInterface,
    public Resource,
    public Loadable,
    public Managed<Mesh>,
    public generic::Identifiable<MeshID>,
    public KeyFrameAnimated,
    public std::enable_shared_from_this<Mesh> {

    DEFINE_SIGNAL(SignalAnimationEnabled, signal_animation_enabled);

public:
    Mesh(MeshID id,
         ResourceManager* resource_manager,
         VertexSpecification vertex_specification);

    ~Mesh();

    void reset(VertexSpecification vertex_specification);

    SubMesh* new_submesh_with_material(
        const std::string& name,
        MaterialID material,        
        MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES,
        VertexSharingMode vertex_sharing=VERTEX_SHARING_MODE_SHARED,
        VertexSpecification vertex_specification=VertexSpecification()
    );

    SubMesh* new_submesh(
        const std::string& name,
        MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES,
        VertexSharingMode vertex_sharing=VERTEX_SHARING_MODE_SHARED,
        VertexSpecification vertex_specification=VertexSpecification()
    );

    SubMesh* new_submesh_as_rectangle(
        const std::string& name,
        MaterialID material,
        float width,
        float height,
        const Vec3& offset=Vec3()
    );

    SubMesh* new_submesh_as_box(
        const std::string& name,
        MaterialID material,
        float width,
        float height,
        float depth,
        const Vec3& offset=Vec3()
    );

    uint32_t submesh_count() const { return submeshes_.size(); }
    bool has_submesh(const std::string& name) const { return submeshes_.count(name); }
    SubMesh* submesh(const std::string& name);
    SubMesh* first_submesh() const;

    void delete_submesh(const std::string& name);
    void clear();

    void enable_debug(bool value);

    void set_material_id(MaterialID material); ///< Apply material to all submeshes
    void set_diffuse(const kglt::Colour& colour, bool include_submeshes=true); ///< Override vertex colour on all vertices

    void reverse_winding(); ///< Reverse the winding of all submeshes
    void set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass=0); ///< Replace the texture unit on all submesh materials

    const AABB aabb() const;
    void normalize(); //Scales the mesh so it has a radius of 1.0
    void transform_vertices(const kglt::Mat4& transform, bool include_submeshes=true);

    void each(std::function<void (const std::string&, SubMesh*)> func) const;

    void enable_animation(MeshAnimationType animation_type, uint32_t animation_frames);
    bool is_animated() const { return animation_type_ != MESH_ANIMATION_TYPE_NONE; }
    uint32_t animation_frames() const { return animation_frames_; }
    MeshAnimationType animation_type() const { return animation_type_; }

public:
    // Signals

    typedef sig::signal<void (MeshID, SubMesh*)> SubMeshCreatedCallback;
    typedef sig::signal<void (MeshID, SubMesh*)> SubMeshDestroyedCallback;
    typedef sig::signal<void (MeshID, SubMesh*, MaterialID, MaterialID)> SubMeshMaterialChangedCallback;

    SubMeshCreatedCallback& signal_submesh_created() { return signal_submesh_created_; }
    SubMeshDestroyedCallback& signal_submesh_destroyed() { return signal_submesh_destroyed_; }
    SubMeshMaterialChangedCallback& signal_submesh_material_changed() { return signal_submesh_material_changed_; }

private:
    friend class SubMesh;
    VertexData* get_shared_data() const;

    bool shared_data_dirty_ = false;
    VertexData* shared_data_ = nullptr;
    MeshAnimationType animation_type_ = MESH_ANIMATION_TYPE_NONE;
    uint32_t animation_frames_ = 0;

#ifdef KGLT_GL_VERSION_2X
    void _update_buffer_object();
    BufferObjectPtr shared_data_buffer_object_;
#endif

    std::unordered_map<std::string, std::shared_ptr<SubMesh>> submeshes_;

    SubMesh* normal_debug_mesh_ = nullptr;

    SubMeshCreatedCallback signal_submesh_created_;
    SubMeshDestroyedCallback signal_submesh_destroyed_;
    SubMeshMaterialChangedCallback signal_submesh_material_changed_;
};

}

#endif // NEWMESH_H
