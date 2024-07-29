#include "node_locator.h"

std::tuple<smlt::NodeFinder, smlt::StageNode*, smlt::StageNodeNotificationList>
    smlt::FindDescendent(const char* name, StageNode* behaviour) {
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

std::tuple<smlt::NodeFinder, smlt::StageNode*, smlt::StageNodeNotificationList>
    smlt::FindAncestor(const char* name, StageNode* behaviour) {
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
