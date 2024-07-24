#include "stage_node_watch_controller.h"

namespace smlt {

int StageNodeWatchController::id_counter_ = 0;

void StageNodeWatchController::_signal_change(StageNode* node,
                                              StageNodePath old_path,
                                              StageNodePath new_path,
                                              StageNodeChange change) {

    // if it's an attachment, we need the new path to match against watchers
    // if it's a detachment, we need to notify watchers of the old path
    for(auto& p: watchers_) {
        if(old_path == new_path) {
            assert(change == STAGE_NODE_CHANGE_MIXINS);
            p.second.func(STAGE_NODE_NOTIFICATION_TARGET_MIXINS_CHANGED, node);
        } else if(p.first == old_path) {
            p.second.func(STAGE_NODE_NOTIFICATION_TARGET_DETACHED, node);
        } else if(p.first.starts_with(old_path)) {
            p.second.func(STAGE_NODE_NOTIFICATION_ANCESTOR_DETACHED, node);
        } else if(old_path.starts_with(p.first)) {
            p.second.func(STAGE_NODE_NOTIFICATION_DESCENDENT_DETACHED, node);
            if(old_path.length() == p.first.length() + 1) {
                p.second.func(STAGE_NODE_NOTIFICATION_CHILD_DETACHED, node);
            }
        } else if(p.first == new_path) {
            p.second.func(STAGE_NODE_NOTIFICATION_TARGET_ATTACHED, node);
        } else if(p.first.starts_with(new_path)) {
            p.second.func(STAGE_NODE_NOTIFICATION_ANCESTOR_ATTACHED, node);
        } else if(new_path.starts_with(p.first)) {
            p.second.func(STAGE_NODE_NOTIFICATION_DESCENDENT_ATTACHED, node);
            if(p.first.length() == new_path.length() + 1) {
                p.second.func(STAGE_NODE_NOTIFICATION_CHILD_ATTACHED, node);
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
