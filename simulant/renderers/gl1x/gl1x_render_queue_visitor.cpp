
#ifdef _arch_dreamcast
    #include <GL/gl.h>
    #include <GL/glext.h>
#else
    #include "./glad/glad/glad.h"
#endif

#include "gl1x_render_queue_visitor.h"
#include "gl1x_renderer.h"
#include "gl1x_render_group_impl.h"

#include "../../camera.h"
#include "../../utils/gl_error.h"

namespace smlt {


GL1RenderQueueVisitor::GL1RenderQueueVisitor(GL1XRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void GL1RenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage) {
    enable_vertex_arrays(true);
    enable_colour_arrays(true);
}

void GL1RenderQueueVisitor::visit(Renderable* renderable, MaterialPass* pass, batcher::Iteration iteration) {
    queue_blended_objects_ = true;
    do_visit(renderable, pass, iteration);
}

void GL1RenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, Stage* stage) {
    // When running do_visit, don't queue blended objects just render them
    queue_blended_objects_ = false;

    // Should be ordered by distance to camera
    for(auto p: blended_object_queue_) {
        GL1RenderState& state = p.second;

        if(state.render_group_impl != current_group_) {
            // Make sure we change render group (shaders, textures etc.)
            batcher::RenderGroup prev(current_group_->shared_from_this());
            batcher::RenderGroup next(state.render_group_impl->shared_from_this());

            change_render_group(&prev, &next);
            current_group_ = state.render_group_impl;
        }

        if(pass_ != state.pass) {
            change_material_pass(pass_, state.pass);
        }

        // FIXME: Pass the previous light from the last iteration, not nullptr
        change_light(nullptr, state.light);

        // Render the transparent / blended objects
        do_visit(
            state.renderable,
            state.pass,
            state.iteration
        );
    }

    blended_object_queue_.clear();
    queue_blended_objects_ = true;
}

void GL1RenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {
    // Casting blindly because I can't see how it's possible that it's anything else!
    auto last_group = (prev) ? (GL1RenderGroupImpl*) prev->impl() : nullptr;
    current_group_ = (GL1RenderGroupImpl*) next->impl();

    // Set up the textures appropriately depending on the group textures
    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        auto current_tex = current_group_->texture_id[i];
        if(!last_group || last_group->texture_id[i] != current_tex) {            
            GLCheck(glEnable, GL_TEXTURE_2D);
            GLCheck(glActiveTextureARB, GL_TEXTURE0_ARB + i);
            if(current_tex) {
                //L_DEBUG(_F("Binding texture {0} to texture unit {1}").format(current_tex, i));
                GLCheck(glBindTexture, GL_TEXTURE_2D, current_tex);
            } else {
                //L_DEBUG(_F("Disabling texture unit {0}").format(i));
                GLCheck(glBindTexture, GL_TEXTURE_2D, 0);
                //GLCheck(glDisable, GL_TEXTURE_2D);
            }
        }
    }
}

void GL1RenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    pass_ = next;

    if(!prev || prev->depth_test_enabled() != next->depth_test_enabled()) {
        if(next->depth_test_enabled()) {
            GLCheck(glEnable, GL_DEPTH_TEST);
        } else {
            GLCheck(glDisable, GL_DEPTH_TEST);
        }
    }

    if(!prev || prev->depth_write_enabled() != next->depth_write_enabled()) {
        if(next->depth_write_enabled()) {
            GLCheck(glDepthMask, GL_TRUE);
        } else {
            GLCheck(glDepthMask, GL_FALSE);
        }
    }

#ifndef _arch_dreamcast
    if(!prev || prev->point_size() != next->point_size()) {
        glPointSize(next->point_size());
    }

    if(!prev || prev->polygon_mode() != next->polygon_mode()) {
        switch(next->polygon_mode()) {
            case POLYGON_MODE_POINT:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
            case POLYGON_MODE_LINE:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
            default:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
#endif

    if(!prev || prev->cull_mode() != next->cull_mode()) {
        if(next->cull_mode() != CULL_MODE_NONE) {
            glEnable(GL_CULL_FACE);
        }

        switch(next->cull_mode()) {
            case CULL_MODE_NONE:
                glDisable(GL_CULL_FACE);
            break;
            case CULL_MODE_FRONT_FACE:
                glCullFace(GL_FRONT);
            break;
            case CULL_MODE_BACK_FACE:
                glCullFace(GL_BACK);
            break;
            case CULL_MODE_FRONT_AND_BACK_FACE:
                glCullFace(GL_FRONT_AND_BACK);
            break;
        }
    }

    auto set_blending_mode = [](BlendType type) {
        if(type == BLEND_NONE) {
            GLCheck(glDisable, GL_BLEND);
            return;
        }

        GLCheck(glEnable, GL_BLEND);
        switch(type) {
            case BLEND_ADD: GLCheck(glBlendFunc, GL_ONE, GL_ONE);
            break;
            case BLEND_ALPHA: GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
            case BLEND_COLOUR: GLCheck(glBlendFunc, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
            break;
            case BLEND_MODULATE: GLCheck(glBlendFunc, GL_DST_COLOR, GL_ZERO);
            break;
            case BLEND_ONE_ONE_MINUS_ALPHA: GLCheck(glBlendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        default:
            throw std::logic_error("Invalid blend type specified");
        }
    };

    if(!prev || prev->blending() != next->blending()) {
        set_blending_mode(next->blending());
    }
}

void GL1RenderQueueVisitor::change_light(const Light* prev, const Light* next) {

}

bool GL1RenderQueueVisitor::queue_if_blended(Renderable* renderable, MaterialPass* material_pass, batcher::Iteration iteration) {
    if(material_pass->is_blended() && queue_blended_objects_) {
        auto pos = renderable->transformed_aabb().centre();
        auto plane = camera_->frustum().plane(FRUSTUM_PLANE_NEAR);

        float key = plane.distance_to(pos);

        GL1RenderState state;
        state.renderable = renderable;
        state.pass = material_pass;
        state.light = light_;
        state.iteration = iteration;
        state.render_group_impl = current_group_;

        blended_object_queue_.insert(
            std::make_pair(key, state)
        );

        // We are done for now, we'll render this in back-to-front order later
        return true;
    } else {
        return false;
    }
}

void GL1RenderQueueVisitor::enable_vertex_arrays(bool force) {
    if(!force && positions_enabled_) return;
    GLCheck(glEnableClientState, GL_VERTEX_ARRAY);
    positions_enabled_ = true;
}

void GL1RenderQueueVisitor::disable_vertex_arrays(bool force) {
    if(!force && !positions_enabled_) return;

    GLCheck(glDisableClientState, GL_VERTEX_ARRAY);
    positions_enabled_ = false;
}

void GL1RenderQueueVisitor::enable_colour_arrays(bool force) {
    if(!force && colours_enabled_) return;
    GLCheck(glEnableClientState, GL_COLOR_ARRAY);
    colours_enabled_ = true;
}

void GL1RenderQueueVisitor::disable_colour_arrays(bool force) {
    if(!force && !colours_enabled_) return;

    GLCheck(glDisableClientState, GL_COLOR_ARRAY);
    colours_enabled_ = false;
}

void GL1RenderQueueVisitor::enable_texcoord_array(uint8_t which, bool force) {
    if(!force && textures_enabled_[which]) return;

    GLCheck(glClientActiveTextureARB, GL_TEXTURE0_ARB + which);
    GLCheck(glEnableClientState, GL_TEXTURE_COORD_ARRAY);
    textures_enabled_[which] = true;
}

void GL1RenderQueueVisitor::disable_texcoord_array(uint8_t which, bool force) {
    if(!force && !textures_enabled_[which]) return;

    GLCheck(glClientActiveTextureARB, GL_TEXTURE0_ARB + which);
    GLCheck(glDisableClientState, GL_TEXTURE_COORD_ARRAY);
    textures_enabled_[which] = false;
}

GLenum convert_arrangement(MeshArrangement arrangement) {
    switch(arrangement) {
    case MESH_ARRANGEMENT_POINTS:
        return GL_POINTS;
    case MESH_ARRANGEMENT_LINES:
        return GL_LINES;
    case MESH_ARRANGEMENT_LINE_STRIP:
        return GL_LINE_STRIP;
    case MESH_ARRANGEMENT_TRIANGLES:
        return GL_TRIANGLES;
    case MESH_ARRANGEMENT_TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;
    case MESH_ARRANGEMENT_TRIANGLE_FAN:
        return GL_TRIANGLE_FAN;
    default:
        throw std::runtime_error("Invalid vertex arrangement");
    }
}

static GLenum convert_index_type(IndexType type) {
    switch(type) {
    case INDEX_TYPE_8_BIT: return GL_UNSIGNED_BYTE;
    case INDEX_TYPE_16_BIT: return GL_UNSIGNED_SHORT;
    case INDEX_TYPE_32_BIT: return GL_UNSIGNED_INT;
    default:
        throw std::logic_error("Invalid index type");
    }
}

void GL1RenderQueueVisitor::do_visit(Renderable* renderable, MaterialPass* material_pass, batcher::Iteration iteration) {
    if(queue_if_blended(renderable, material_pass, iteration)) {
        // If this was a transparent object, and we were queuing then do nothing else for now
        return;
    }

    auto element_count = renderable->index_element_count();
    // Don't bother doing *anything* if there is nothing to render
    if(!element_count) {
        return;
    }

    const Mat4 model = renderable->final_transformation();
    const Mat4& view = camera_->view_matrix();
    const Mat4& projection = camera_->projection_matrix();

    Mat4 modelview = view * model;

    GLCheck(glMatrixMode, GL_MODELVIEW);
    GLCheck(glLoadMatrixf, (const GLfloat*) &modelview);

    GLCheck(glMatrixMode, GL_PROJECTION);
    GLCheck(glLoadMatrixf, (const GLfloat*) &projection);

    auto spec = renderable->vertex_attribute_specification();

    renderable->prepare_buffers();

    /* We need to get access to the vertex data that's been uploaded, and map_target_for_read is the only way to do that
     * but as on GL1 there are no VBOs this should be fast */
    auto vertex_data = renderable->vertex_attribute_buffer()->map_target_for_read();
    auto index_data = renderable->index_buffer()->map_target_for_read();

    (spec.has_positions()) ? enable_vertex_arrays() : disable_vertex_arrays();
    (spec.has_diffuse()) ? enable_colour_arrays() : disable_colour_arrays();

    GLCheck(
        glVertexPointer,
        (spec.position_attribute == VERTEX_ATTRIBUTE_2F) ? 2 : (spec.position_attribute == VERTEX_ATTRIBUTE_3F) ? 3 : 4,
        GL_FLOAT,
        spec.stride(),
        ((const uint8_t*) vertex_data) + spec.position_offset(false)
    );

    const uint8_t* colour_pointer = (spec.has_diffuse()) ?
        ((const uint8_t*) vertex_data) + spec.diffuse_offset(false) :
        nullptr;

    GLCheck(
        glColorPointer,
        (spec.diffuse_attribute == VERTEX_ATTRIBUTE_2F) ? 2 : (spec.diffuse_attribute == VERTEX_ATTRIBUTE_3F) ? 3 : 4,
        GL_FLOAT,
        spec.stride(),
        colour_pointer
    );


    for(uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        bool enabled = spec.has_texcoordX(i);
        auto offset = spec.texcoordX_offset(i, false);
        const uint8_t* coord_pointer = (enabled) ?
            ((const uint8_t*) vertex_data) + offset :
            nullptr;

        (enabled) ? enable_texcoord_array(i) : disable_texcoord_array(i);

        if(enabled) {
            GLCheck(glClientActiveTextureARB, GL_TEXTURE0_ARB + i);
            GLCheck(
                glTexCoordPointer,
                (spec.texcoordX_attribute(i) == VERTEX_ATTRIBUTE_2F) ? 2 : (spec.texcoordX_attribute(i) == VERTEX_ATTRIBUTE_3F) ? 3 : 4,
                GL_FLOAT,
                spec.stride(),
                coord_pointer
            );
        }
    }

    auto arrangement = convert_arrangement(renderable->arrangement());
    GLCheck(
        glDrawElements,
        arrangement,
        element_count,
        convert_index_type(renderable->index_type()),
        (const void*) index_data
    );
}

}
