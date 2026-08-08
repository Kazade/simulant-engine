#include "prefab.h"

namespace smlt {

void Prefab::push_node(PrefabNode node, int32_t parent_id) {
    PrefabKey parent;

    // if there is a parent, find the existing node with the parent id
    // and build a path to the node
    if(parent_id != -1) {
        for(auto& p: nodes_) {
            if(p.first.path[p.first.path.size() - 1] == (uint32_t)parent_id) {
                parent = p.first;
            }
        }
    }

    parent.path.push_back(node.id);
    nodes_.insert(std::make_pair(parent, node));
}

uint64_t Prefab::estimated_size_in_bytes() const {
    uint64_t total = sizeof(*this);

    /* nodes_/animations_ are std::multimap - a red-black tree where each
     * entry is a separately heap-allocated node. ~3 pointers + a colour bit
     * is a reasonable estimate of the per-node overhead on top of the
     * stored key/value pair itself (which includes each PrefabNode's
     * embedded Params by value). */
    const uint64_t rb_node_overhead = sizeof(void*) * 3;

    /* Each param value is itself a separately heap-allocated std::map node
     * inside Params - counted on top of the (empty-map-sized) PrefabNode
     * above. Note this doesn't account for heap data *inside* a param
     * value (e.g. a long std::string or FloatArray) - good enough for
     * spotting nodes with an unusually large number of params, not exact. */
    const uint64_t param_node_overhead = sizeof(void*) * 3 + sizeof(ParamKey);

    for(auto& entry: nodes_) {
        total += sizeof(entry) + rb_node_overhead;
        total += entry.second.params.size() * param_node_overhead;
    }

    for(auto& entry: animations_) {
        total += sizeof(entry) + rb_node_overhead;
        total += entry.second.target.params.size() * param_node_overhead;
    }

    /* textures_/materials_/meshes_ only reference assets that are already
     * logged (and sized) independently elsewhere - count the pointer
     * storage itself, not what they point to, to avoid double-counting. */
    total += textures_.size() * sizeof(TexturePtr);
    total += materials_.size() * sizeof(MaterialPtr);
    total += meshes_.size() * sizeof(MeshPtr);

    return total;
}

} // namespace smlt
