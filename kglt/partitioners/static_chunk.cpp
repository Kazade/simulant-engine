#include "../stage.h"
#include "static_chunk.h"

namespace kglt {

StaticSubchunk::StaticSubchunk(StaticChunk* parent, RenderPriority priority, MaterialID material_id, MeshArrangement arrangement):
    parent_(parent),
    mesh_(parent->mesh.get()),
    render_priority_(priority) {

    auto smi = mesh_->new_submesh(arrangement, VERTEX_SHARING_MODE_INDEPENDENT);
    submesh_ = mesh_->submesh(smi);
    submesh_->set_material_id(material_id);
}


StaticChunk::StaticChunk(Stage* stage):
    mesh_(MeshID(), stage) {

}

StaticSubchunk* StaticChunk::get_or_create_subchunk(KeyType key) {
    if(!subchunks_.count(key)) {
        subchunks_.insert(std::make_pair(key, std::make_shared<StaticSubchunk>(this, std::get<0>(key), std::get<1>(key), std::get<2>(key))));
    }

    return subchunks_[key].get();
}


}
