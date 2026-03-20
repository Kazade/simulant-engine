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
        if(stack_buffer_tail_ == stack_buffer_.size()) {
            S_WARN("StageNodeVisitorBFS queue overflow, some nodes may not be "
                   "visited");
            return;
        }

        stack_buffer_[stack_buffer_tail_++] = node;
    }

    StageNode* queue_pop() {
        StageNode* ret = stack_buffer_[stack_buffer_head_++];
        if(stack_buffer_head_ == stack_buffer_tail_) {
            stack_buffer_head_ = 0;
            stack_buffer_tail_ = 0;
        }
        return ret;
    }

    bool queue_empty() const {
        return stack_buffer_head_ == stack_buffer_tail_;
    }

    std::function<void(StageNode*)> callback_;
    std::array<StageNode*, 128> stack_buffer_;
    std::size_t stack_buffer_head_ = 0;
    std::size_t stack_buffer_tail_ = 0;
};

} // namespace smlt
