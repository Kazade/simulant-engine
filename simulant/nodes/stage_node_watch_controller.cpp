#include "stage_node_watch_controller.h"

namespace smlt {

int StageNodeWatchController::id_counter_ = 0;

void StageNodeWatchController::_signal_change(StageNodePath node_path,
                                              StageNodeChange change) {
    for(auto& p: watchers_) {
        if(p.first == node_path) {
            if(change == STAGE_NODE_CHANGE_ATTACHED) {
                p.second.func(STAGE_NODE_NOTIFICATION_TARGET_ATTACHED);
            } else {
                p.second.func(STAGE_NODE_NOTIFICATION_TARGET_DETACHED);
            }
        } else if(p.first.starts_with(node_path)) {
            if(change == STAGE_NODE_CHANGE_ATTACHED) {
                p.second.func(STAGE_NODE_NOTIFICATION_DESCENDENT_ATTACHED);
            } else {
                p.second.func(STAGE_NODE_NOTIFICATION_DESCENDENT_DETACHED);
            }
        } else if(node_path.starts_with(p.first)) {
            if(change == STAGE_NODE_CHANGE_ATTACHED) {
                p.second.func(STAGE_NODE_NOTIFICATION_ANCESTOR_ATTACHED);
            } else {
                p.second.func(STAGE_NODE_NOTIFICATION_ANCESTOR_DETACHED);
            }
        }
    }
}

bool StageNodeWatchConnection::disconnect() {
    if(ref_.lock()) {
        auto range = controller_->watchers_.equal_range(path_);
        for(auto it = range.first; it != range.second; ++it) {
            if(it->second.id == id_) {
                controller_->watchers_.erase(it);
                id_ = 0;
                return true;
            }
        }
    }

    return false;
}

} // namespace smlt
