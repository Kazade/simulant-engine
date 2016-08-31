#include "ownable.h"
#include "../interfaces.h"

namespace kglt {

void ownable_tree_node_destroy(GenericTreeNode* node) {
    Ownable* ownable = dynamic_cast<Ownable*>(node);

    if(!ownable) {
        L_WARN("Tried to destroy a tree node that is not an Ownable");
        return;
    }

    ownable->ask_owner_for_destruction();
}

}
