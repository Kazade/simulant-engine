#pragma once

#include "renderer_config.h"

namespace kglt {

const uint32_t RENDER_PRIORITY_RANGE = RENDER_PRIORITY_MAX - RENDER_PRIORITY_ABSOLUTE_BACKGROUND;
const uint32_t MAX_MATERIAL_PASSES = 25;

struct RenderQueue {
    RootGroup* passes_[MAX_MATERIAL_PASSES] = {nullptr};

    RootGroup* get_or_create_pass(uint32_t pass, WindowBase* window, StageID stage_id, CameraID camera_id) {
        assert(pass < MAX_MATERIAL_PASSES);
        if(!passes_[pass]) {
            passes_[pass] = new RootGroup(*window, stage_id, camera_id);
        }
        return passes_[pass];
    }

    ~RenderQueue() {
        for(uint32_t i = 0; i < MAX_MATERIAL_PASSES; ++i) {
            if(passes_[i]) {
                delete passes_[i];
                passes_[i] = nullptr;
            }
        }
    }

    void each(std::function<void(RootGroup*)> func) {
        for(RootGroup* group: passes_) {
            if(group) {
                func(group);
            }
        }
    }
};

struct QueuesByPriority {
    /*
     * This is a set of render queues indexed by their render priority
     * Everything is raw pointers and static arrays for performance.
     *
     * It might be a wise idea to just allocate the entire range on construction
     * but this is fast enough for now
     */
    RenderQueue* queues_[RENDER_PRIORITY_RANGE] = {nullptr};

    RenderQueue* get_or_create_queue(RenderPriority priority) {
        uint32_t idx = uint32_t(priority - RENDER_PRIORITY_ABSOLUTE_BACKGROUND);

        assert(idx < RENDER_PRIORITY_RANGE);

        if(!queues_[idx]) {
            queues_[idx] = new RenderQueue();
        }

        return queues_[idx];
    }

    void clear() {
        for(uint32_t i = 0; i < RENDER_PRIORITY_RANGE; ++i) {
            if(queues_[i]) {
                delete queues_[i];
                queues_[i] = nullptr;
            }
        }
    }

    ~QueuesByPriority() {
        clear();
    }

    void each(std::function<void(RenderQueue*)> func) {
        for(auto queue: queues_) {
            if(queue) {
                func(queue);
            }
        }
    }
};

}
