#include "node_locator.h"

namespace smlt {

std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindDescendent(const char* name, StageNode* behaviour) {
    StageNodeNotificationList invalidation_messages = {
        STAGE_NODE_NOTIFICATION_DESCENDENT_ATTACHED,
        STAGE_NODE_NOTIFICATION_DESCENDENT_DETACHED,
        STAGE_NODE_NOTIFICATION_TARGET_ATTACHED,
        STAGE_NODE_NOTIFICATION_TARGET_DETACHED,
    };

    return {std::bind(&StageNodeFinders::find_descendent, name,
                      std::placeholders::_1),
            behaviour, invalidation_messages};
}

std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindDescendentByID(StageNodeID id, StageNode* behaviour) {
    StageNodeNotificationList invalidation_messages = {
        STAGE_NODE_NOTIFICATION_DESCENDENT_ATTACHED,
        STAGE_NODE_NOTIFICATION_DESCENDENT_DETACHED,
        STAGE_NODE_NOTIFICATION_TARGET_ATTACHED,
        STAGE_NODE_NOTIFICATION_TARGET_DETACHED,
    };

    return {std::bind(&StageNodeFinders::find_descendent_by_id, id,
                      std::placeholders::_1),
            behaviour, invalidation_messages};
}

std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindAncestor(const char* name, StageNode* behaviour) {
    StageNodeNotificationList invalidation_messages = {
        STAGE_NODE_NOTIFICATION_ANCESTOR_ATTACHED,
        STAGE_NODE_NOTIFICATION_ANCESTOR_DETACHED,
        STAGE_NODE_NOTIFICATION_TARGET_ATTACHED,
        STAGE_NODE_NOTIFICATION_TARGET_DETACHED,
    };

    return {std::bind(&StageNodeFinders::find_ancestor, name,
                      std::placeholders::_1),
            behaviour, invalidation_messages};
}

} // namespace smlt
