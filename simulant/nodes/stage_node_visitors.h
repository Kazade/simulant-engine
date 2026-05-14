#pragma once

#include <vector>
#include "stage_node.h"
#include "stage_node_iterators.h"

namespace smlt {

/* BFS traversal over a stage node subtree.
 *
 * Avoids std::function (heap alloc + virtual dispatch) and the per-node
 * call_next() loop by accepting the callback as a concrete template type.
 * The queue buffer is a function-static so it retains capacity between
 * frames, growing once to fit the scene and never reallocating again. */
template<typename Func>
inline void traverse_bfs(StageNode* start, Func&& func) {
    static std::vector<StageNode*> bfs_queue;
    bfs_queue.clear();
    bfs_queue.push_back(start);

    for(std::size_t i = 0; i < bfs_queue.size(); ++i) {
        StageNode* node = bfs_queue[i];
        for(auto& child: node->each_child()) {
            bfs_queue.push_back(&child);
        }
        func(node);
    }
}

} // namespace smlt
