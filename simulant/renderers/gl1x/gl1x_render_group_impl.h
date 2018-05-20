#pragma once

#include <memory>

#ifdef _arch_dreamcast
    #include "../../../deps/libgl/include/gl.h"
#else
    #include "./glad/glad/glad.h"
#endif

#include "../../material.h"

namespace smlt {

class GL1RenderGroupImpl:
    public batcher::RenderGroupImpl,
    public std::enable_shared_from_this<GL1RenderGroupImpl> {

public:
    GL1RenderGroupImpl(RenderPriority priority):
        batcher::RenderGroupImpl(priority) {}

    GLuint texture_id[MAX_TEXTURE_UNITS] = {0};

    bool lt(const RenderGroupImpl& other) const override {
        const GL1RenderGroupImpl* rhs = dynamic_cast<const GL1RenderGroupImpl*>(&other);
        if(!rhs) {
            // Should never happen... throw an error maybe?
            return false;
        }

        for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            if(texture_id[i] < rhs->texture_id[i]) {
                return true;
            } else if(texture_id[i] > rhs->texture_id[i]) {
                return false;
            }
        }

        return false;
    }
};


}
