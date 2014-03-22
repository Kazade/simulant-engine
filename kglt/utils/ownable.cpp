#include "ownable.h"
#include "../interfaces.h"
#include "kazbase/exceptions.h"

namespace kglt {

void ownable_tree_node_destroy(GenericTreeNode* node) {
    Ownable* ownable = dynamic_cast<Ownable*>(node);

    if(!ownable) {
        throw LogicError("Tried to destroy a tree node that is not an Ownable");
    }

    ownable->ask_owner_for_destruction();
}

}
