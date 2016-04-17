#pragma once

#include "../generic/property.h"
#include "../mesh.h"

namespace kglt {

class StaticChunk;

class StaticSubchunk:
    public Renderable {

public:
    typedef std::shared_ptr<StaticSubchunk> ptr;

    StaticSubchunk(StaticChunk* parent, RenderPriority priority, MaterialID material_id, MeshArrangement arrangement);
    ~StaticSubchunk() {
        mesh_->delete_submesh(submesh_->id());
    }

    const AABB aabb() const { return submesh_->aabb(); }
    const AABB transformed_aabb() const { return submesh_->aabb(); }
    const VertexData& vertex_data() const { return submesh_->vertex_data(); }
    const IndexData& index_data() const { return submesh_->index_data(); }
    const MeshArrangement arrangement() const { return submesh_->arrangement(); }

    void _update_vertex_array_object() { submesh_->_update_vertex_array_object(); }
    void _bind_vertex_array_object() { submesh_->_bind_vertex_array_object(); }

    RenderPriority render_priority() const { return render_priority_; }

    Mat4 final_transformation() const { return Mat4(); }
    const MaterialID material_id() const { return submesh_->material_id(); }
    const bool is_visible() const { return true; }
    MeshID instanced_mesh_id() const { return mesh_->id(); }
    SubMeshID instanced_submesh_id() const { return submesh_->id(); }

private:
    StaticChunk* parent_;
    Mesh* mesh_;
    RenderPriority render_priority_;
    SubMesh* submesh_;
};

class StaticChunk {
public:
    typedef std::tuple<RenderPriority, MaterialID, MeshArrangement> KeyType;

    StaticChunk(Stage* stage);

    typedef std::shared_ptr<StaticChunk> ptr;

    StaticSubchunk* get_or_create_subchunk(KeyType key);

    Property<StaticChunk, Mesh> mesh = { this, &StaticChunk::mesh_ };
private:

    typedef std::unordered_map<KeyType, StaticSubchunk::ptr> SubchunkLookup;

    SubchunkLookup subchunks_;
    Mesh mesh_;
};

struct StaticChunkHolder {
    typedef std::shared_ptr<StaticChunkHolder> ptr;

    std::unordered_map<GeomID, StaticChunk::ptr> chunks;
};



}
