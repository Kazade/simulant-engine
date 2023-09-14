#pragma once

#include <cstddef>
#include <functional>
#include "../../nodes/stage_node.h"

namespace smlt {

class StageNodeFinders {
    static StageNode* find_descendent(const char* name, StageNode* organism) {
        for(auto& node: organism->each_descendent()) {
            if(node.name() == name) {
                return &node;
            }
        }

        return nullptr;
    }

    static StageNode* find_ancestor(const char* name, StageNode* organism){
        auto parent = (StageNode*) organism->parent();
        while(parent) {
            if(parent->name() == name) {
                return parent;
            }

            parent = (StageNode*) parent->parent();
        }

        return parent;
    }
};

typedef std::function<StageNode* (StageNode*)> NodeFinder;

template<typename T>
class FindResult {
public:
    FindResult(const std::tuple<NodeFinder, Behaviour*>& finder):
        finder_(std::get<0>(finder)),
        behaviour_(std::get<1>(finder)) {
    }

    ~FindResult() {
        if(on_destroy_.is_connected())
            on_destroy_.disconnect();
    }

    bool operator==(const StageNode* other) const {
        return get() == other;
    }

    bool operator!=(const StageNode* other) const {
        return get() != other;
    }

    operator bool() const {
        return bool(this->operator ->());
    }

    bool operator==(std::nullptr_t) const {
        return !this->operator ->();
    }

    operator T*() {
        return this->operator ->();
    }

    bool is_cached() {
        return bool(found_);
    }

    T* get() {
        return this->operator ->();
    }

    const T* get() const {
        return this->operator ->();
    }

    T* operator->() {
        return const_cast<T*>(static_cast<const FindResult*>(this)->operator ->());
    }

    const T* operator->() const {
        if(found_) {
            return found_;
        } else {
            if(!organism_) {
                /* Only dynamic cast in debug mode. In release just blindly cast
                 * and expect it'll work */
#ifndef NDEBUG
                organism_ = dynamic_cast<StageNode*>(behaviour_->organism.get());
                assert(organism_);
#else
                organism_ = (StageNode*) behaviour_->organism.get();
#endif
                if(!organism_) {
                    S_WARN("Couldn't locate node because behaviour is not attached");
                    return nullptr;
                }
            }

            found_ = dynamic_cast<T*>(finder_(organism_));
            if(found_ && !found_->is_destroyed()) {
                on_destroy_ = found_->signal_destroyed().connect([&]() {
                    found_ = nullptr;
                    on_destroy_.disconnect();
                });
            }
            return found_;
        }
    }

private:
    std::function<StageNode* (StageNode*)> finder_;
    Behaviour* behaviour_ = nullptr;

    /* Mutable, because these are only populated on first-access and that
     * first access might be const */
    mutable StageNode* organism_ = nullptr;
    mutable T* found_ = nullptr;
    mutable sig::connection on_destroy_;
};

std::tuple<NodeFinder, Behaviour*> FindAncestor(const char* name, Behaviour* behaviour) {
    return {
        std::bind(&StageNodeFinders::find_ancestor, name, std::placeholders::_1),
        behaviour
    };
}

std::tuple<NodeFinder, Behaviour*> FindDescendent(const char* name, Behaviour* behaviour) {
    return {
        std::bind(&StageNodeFinders::find_descendent, name, std::placeholders::_1),
        behaviour
    };
}

}
