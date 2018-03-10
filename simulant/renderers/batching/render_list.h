#pragma once

#include <set>
#include <vector>
#include <memory>

#include "../../types.h"
#include "render_queue.h"


#ifdef _arch_dreamcast
// Dreamcast has more limited ram
#define MAX_NODES_PER_BATCH 4096
#else
#define MAX_NODES_PER_BATCH 16384
#endif

namespace smlt {

class HardwareBuffer;

/* A single renderable thing */
struct RenderNode {
    MaterialID material_id;
    MeshArrangement arrangement;
    IndexType index_type;
    std::size_t index_element_count;

    HardwareBuffer* index_buffer = nullptr;
    HardwareBuffer* vertex_buffer = nullptr;

    Mat4 transform;

    /*
     * Set to false if the renderable is invisible
     * or not returned by the partitioner
     */
    bool is_visible = true;
};

struct RenderNodeReference {

}

/* Renderable things grouped by material properties */
struct RenderBatch {
    std::unique_ptr<RenderGroup> group;

    /* Each node is a single draw call */
    std::vector<RenderNode> nodes;

    /* Materials duplicated for cache locality */
    std::unordered_map<MaterialID, Material> materials;
};

typedef std::set<RenderBatch> RenderList;

void traverse_list(const RenderList& list, batcher::RenderQueueVisitor* visitor) {
    visitor->start_traversal()
}

}
