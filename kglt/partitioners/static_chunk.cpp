#include "../stage.h"
#include "static_chunk.h"

namespace kglt {

StaticSubchunk::StaticSubchunk(StaticChunk* parent, RenderPriority priority, MaterialID material_id, MeshArrangement arrangement):
    parent_(parent),
    mesh_(parent->mesh.get()),
    render_priority_(priority) {

    auto smi = mesh_->new_submesh_with_material(material_id, arrangement, VERTEX_SHARING_MODE_INDEPENDENT);
    submesh_ = mesh_->submesh(smi);
}


StaticChunk::StaticChunk(Stage* stage):
    mesh_(MeshID(), stage) {

}

void StaticSubchunk::add_polygon(const Polygon& poly) {
    if(!poly.source_data) {
        L_WARN("Tried to add a polygon without a source data pointer to a StaticSubchunk");
        return;
    }

    for(auto& idx: poly.indices) {
        auto& vert = (*poly.source_data).at(idx);
        submesh_->vertex_data().push(vert);
        submesh_->index_data().push(submesh_->vertex_data().count() - 1);
    }

    submesh_->vertex_data().done();
    submesh_->index_data().done();
}

std::pair<StaticSubchunk *, bool> StaticChunk::get_or_create_subchunk(KeyType key) {
    bool created = false;

    if(!subchunks_.count(key)) {
        subchunks_.insert(std::make_pair(key, std::make_shared<StaticSubchunk>(this, std::get<0>(key), std::get<1>(key), std::get<2>(key))));
        created = true;
    }

    return std::make_pair(subchunks_[key].get(), created);
}


}
