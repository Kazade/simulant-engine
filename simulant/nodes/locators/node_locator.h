#pragma once

#include "../../nodes/stage_node.h"
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
    FindResult(const std::tuple<NodeFinder, StageNode*>& finder) :
        finder_(std::get<0>(finder)), node_(std::get<1>(finder)) {
    }

    ~FindResult() {
        if(on_destroy_.is_connected()) {
            on_destroy_.disconnect();
        }
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
    std::function<StageNode*(StageNode*)> finder_;
    StageNode* node_ = nullptr;

    /* Mutable, because these are only populated on first-access and that
     * first access might be const */
    mutable T* found_ = nullptr;

    /* This is set to true when we've done a search, and set
     * to false if the base node is added or removed, or reparented */
    mutable bool checked_ = false;

    mutable sig::connection on_destroy_;
};

std::tuple<NodeFinder, StageNode*> FindAncestor(const char* name,
                                                StageNode* node);
std::tuple<NodeFinder, StageNode*> FindDescendent(const char* name,
                                                  StageNode* node);

template<typename T>
std::tuple<NodeFinder, StageNode*> FindChild(StageNode* node) {
    return {std::bind(&StageNodeFinders::find_child<T>, std::placeholders::_1),
            node};
}

template<typename T>
std::tuple<NodeFinder, StageNode*> FindMixin(StageNode* node) {
    return {std::bind(&StageNodeFinders::find_mixin<T>, std::placeholders::_1),
            node};
}

} // namespace smlt
