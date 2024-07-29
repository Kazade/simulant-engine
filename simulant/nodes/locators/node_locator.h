#pragma once

#include "../../nodes/stage_node.h"
#include "../../scenes/scene.h"
#include <cstddef>
#include <functional>

namespace smlt {

class StageNodeFinders {
public:
    template<typename T>
    static StageNode* find_child(StageNode* organism) {
        for(auto& node: organism->base()->each_child()) {
            if(node.node_type() == T::Meta::node_type) {
                return &node;
            }
        }

        return nullptr;
    }

    static StageNode* find_descendent(const char* name, StageNode* organism) {
        for(auto& node: organism->base()->each_descendent()) {
            if(node.name() == name) {
                return &node;
            }
        }

        return nullptr;
    }

    static StageNode* find_ancestor(const char* name, StageNode* organism) {
        auto parent = (StageNode*)organism->base()->parent();
        while(parent) {
            if(parent->name() == name) {
                return parent;
            }

            parent = (StageNode*)parent->parent();
        }

        return parent;
    }

    template<typename T>
    static StageNode* find_mixin(StageNode* node) {
        return node->base()->find_mixin<T>();
    }
};

typedef std::function<StageNode*(StageNode*)> NodeFinder;

template<typename T>
class FindResult {
public:
    FindResult(const std::tuple<NodeFinder, StageNode*,
                                StageNodeNotificationList>& finder) :
        finder_(std::get<0>(finder)),
        node_(std::get<1>(finder)),
        notifications_(std::get<2>(finder)) {

        reset_watch();
    }

    ~FindResult() {
        if(on_destroy_.is_connected()) {
            on_destroy_.disconnect();
        }

        connection_.disconnect();
    }

    bool operator==(const StageNode* other) const {
        return get() == other;
    }

    bool operator!=(const StageNode* other) const {
        return get() != other;
    }

    operator bool() const {
        return bool(this->operator->());
    }

    bool operator==(std::nullptr_t) const {
        return !this->operator->();
    }

    operator T*() {
        return this->operator->();
    }

    bool is_cached() {
        return bool(found_);
    }

    T* get() {
        return this->operator->();
    }

    const T* get() const {
        return this->operator->();
    }

    T* operator->() {
        return const_cast<T*>(
            static_cast<const FindResult*>(this)->operator->());
    }

    const T* operator->() const {
        if(!checked_) {
            found_ = dynamic_cast<T*>(finder_(node_));
            if(found_ && !found_->is_destroyed()) {
                on_destroy_ = found_->signal_destroyed().connect([&]() {
                    found_ = nullptr;
                    on_destroy_.disconnect();
                });
            }
            checked_ = true;
        }
        return found_;
    }

private:
    void reset_watch() {
        if(connection_) {
            connection_.disconnect();
        }

        assert(node_);

        connection_ = node_->scene->watch(
            node_->node_path(),
            [=](smlt::StageNodeNotification n, StageNode* node) {
            // We only care about some notifications
            if(std::find(notifications_.begin(), notifications_.end(), n) ==
               notifications_.end()) {
                return;
            }

            checked_ = false;

            // The node that changed was this one, so reset
            // the watch with the new node path
            if(node == node_) {
                reset_watch();
            }
        });
    }

    std::function<StageNode*(StageNode*)> finder_;
    StageNode* node_ = nullptr;

    StageNodeNotificationList notifications_;
    StageNodeWatchConnection connection_;

    /* Mutable, because these are only populated on first-access and that
     * first access might be const */
    mutable T* found_ = nullptr;

    /* This is set to true when we've done a search, and set
     * to false if the base node is added or removed, or reparented */
    mutable bool checked_ = false;

    mutable sig::connection on_destroy_;
};

std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindAncestor(const char* name, StageNode* node);
std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindDescendent(const char* name, StageNode* node);

template<typename T>
std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindChild(StageNode* node) {
    StageNodeNotificationList invalidation_messages = {
        STAGE_NODE_NOTIFICATION_CHILD_ATTACHED,
        STAGE_NODE_NOTIFICATION_CHILD_DETACHED,
        STAGE_NODE_NOTIFICATION_TARGET_ATTACHED,
        STAGE_NODE_NOTIFICATION_TARGET_DETACHED,
    };
    return {std::bind(&StageNodeFinders::find_child<T>, std::placeholders::_1),
            node, invalidation_messages};
}

template<typename T>
std::tuple<NodeFinder, StageNode*, StageNodeNotificationList>
    FindMixin(StageNode* node) {
    StageNodeNotificationList invalidation_messages = {
        STAGE_NODE_NOTIFICATION_TARGET_MIXINS_CHANGED,
    };

    return {std::bind(&StageNodeFinders::find_mixin<T>, std::placeholders::_1),
            node, invalidation_messages};
}

} // namespace smlt
