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

} // namespace smlt
