#pragma once

#include "stage_node_path.h"
#include <functional>
#include <map>

namespace smlt {
enum StageNodeNotification {
    STAGE_NODE_NOTIFICATION_CHILD_ATTACHED,
    STAGE_NODE_NOTIFICATION_CHILD_DETACHED,
    STAGE_NODE_NOTIFICATION_DESCENDENT_ATTACHED,
    STAGE_NODE_NOTIFICATION_DESCENDENT_DETACHED,
    STAGE_NODE_NOTIFICATION_ANCESTOR_DETACHED,
    STAGE_NODE_NOTIFICATION_ANCESTOR_ATTACHED,
    STAGE_NODE_NOTIFICATION_TARGET_DETACHED,
    STAGE_NODE_NOTIFICATION_TARGET_ATTACHED,
    STAGE_NODE_NOTIFICATION_TARGET_MIXINS_CHANGED,
};

enum StageNodeChange {
    STAGE_NODE_CHANGE_HIERARCHY,
    STAGE_NODE_CHANGE_MIXINS
};

typedef std::vector<StageNodeNotification> StageNodeNotificationList;

class StageNode;

typedef std::function<void(StageNodeNotification, StageNode*)> WatchFunc;

class StageNodeWatchController;

class StageNodeWatchConnection {
public:
    StageNodeWatchConnection() = default;
    bool disconnect();

    operator bool() const {
        return is_valid();
    }

    bool is_valid() const {
        return id_ > 0 && ref_.lock();
    }

private:
    friend class StageNodeWatchController;

    std::weak_ptr<bool> ref_;
    StageNodeWatchController* controller_;
    StageNodePath path_;
    int id_ = 0;

    StageNodeWatchConnection(std::weak_ptr<bool> ref,
                             StageNodeWatchController* controller,
                             StageNodePath path, int id) :
        ref_(ref), controller_(controller), path_(path), id_(id) {}
};

class StageNodeWatchController {
public:
    StageNodeWatchConnection watch(StageNodePath path, WatchFunc func) {
        auto entry = WatchEntry(func);
        watchers_.insert(std::make_pair(path, entry));
        return StageNodeWatchConnection(ref_, this, path, entry.id);
    }

private:
    friend class StageNode;
    friend class StageNodeWatchConnection;

    std::shared_ptr<bool> ref_ = std::make_shared<bool>();

    static int id_counter_;

    struct WatchEntry {
        int id = ++id_counter_;
        WatchFunc func;

        WatchEntry(const WatchFunc& func) :
            func(func) {}
    };

    void _signal_change(StageNode* node, StageNodePath old_path,
                        StageNodePath new_path, StageNodeChange change);
    std::multimap<StageNodePath, WatchEntry> watchers_;
};

} // namespace smlt
