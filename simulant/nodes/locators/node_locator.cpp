#include "node_locator.h"

std::tuple<smlt::NodeFinder, smlt::StageNode*> smlt::FindDescendent(const char* name, StageNode* behaviour) {
    return {
        std::bind(&StageNodeFinders::find_descendent, name, std::placeholders::_1),
        behaviour
    };
}

std::tuple<smlt::NodeFinder, smlt::StageNode*> smlt::FindAncestor(const char* name, StageNode* behaviour) {
    return {
        std::bind(&StageNodeFinders::find_ancestor, name, std::placeholders::_1),
        behaviour
    };
}
