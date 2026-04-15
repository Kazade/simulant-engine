#pragma once

#include "stage_node.h"
#include "stage_node_iterators.h"

namespace smlt {

class StageNodeVisitorBFS {
public:
    template<typename Func>
    StageNodeVisitorBFS(StageNode* start, Func&& callback) :
        callback_(callback) {
        queue_push(start);
    }

    bool call_next() {
        StageNode* it = queue_pop();

        for(auto& node: it->each_child()) {
            queue_push(&node);
        }

        callback_(it);

        return !queue_empty();
    }

private:
    void queue_push(StageNode* node) {
        queue_.push_back(node);
    }

    StageNode* queue_pop() {
        return queue_[head_++];
    }

    bool queue_empty() const {
        return head_ >= queue_.size();
    }

    std::function<void(StageNode*)> callback_;
    std::vector<StageNode*> queue_;
    std::size_t head_ = 0;
};

} // namespace smlt
