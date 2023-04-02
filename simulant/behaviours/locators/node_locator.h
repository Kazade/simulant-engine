#pragma once

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
};

template<typename T>
class StageNodeResult {
public:
    template<typename Func>
    StageNodeResult(Func&& finder, Behaviour* behaviour):
        finder_(finder),
        behaviour_(behaviour) {
    }

    bool operator==(nullptr_t) const {
        return !this->operator ->();
    }

    operator T*() {
        return this->operator ->();
    }

    T* get() {
        return this->operator ->();
    }

    T* operator->() {
        return const_cast<T*>(static_cast<const StageNodeResult*>(this)->operator ->());
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

#define S_FIND_DESCENDENT(name) \
    { std::bind(&StageNodeFinders::find_descendent, (name), std::placeholders::_1), this }

}
