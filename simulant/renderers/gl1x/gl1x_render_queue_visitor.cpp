
#ifdef __DREAMCAST__
    #include "../../../deps/libgl/include/gl.h"
    #include "../../../deps/libgl/include/glext.h"
#elif defined(__PSP__)
    #include <GL/gl.h>
#else
    #include "../glad/glad/glad.h"
#endif

#include "gl1x_render_queue_visitor.h"
#include "gl1x_renderer.h"
#include "gl1x_render_group_impl.h"

#include "../../stage.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../utils/gl_error.h"
#include "../../window.h"

namespace smlt {


GL1RenderQueueVisitor::GL1RenderQueueVisitor(GL1XRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void GL1RenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);

    /* Set up default client state before the run. This is necessary
     * so that the boolean flags get correctly set */
    enable_vertex_arrays(true);
    enable_colour_arrays(true);
    enable_normal_arrays(true);
    enable_texcoord_array(0, true);

    for(auto i = 1u; i < _S_GL_MAX_TEXTURE_UNITS; ++i) {
        disable_texcoord_array(i, true);
    }

    global_ambient_ = stage->ambient_light();
    GLCheck(glLightModelfv, GL_LIGHT_MODEL_AMBIENT, &global_ambient_.r);

    if(!stage->fog->is_enabled()) {
        GLCheck(glDisable, GL_FOG);
    } else {
        GLCheck(glEnable, GL_FOG);
        switch(stage->fog->type()) {
        case FOG_TYPE_EXP: {
            GLCheck(glFogi, GL_FOG_MODE, GL_EXP);
            GLCheck(glFogf, GL_FOG_DENSITY, stage->fog->exp_density());
        } break;
        case FOG_TYPE_EXP2: {
            GLCheck(glFogi, GL_FOG_MODE, GL_EXP2);
            GLCheck(glFogf, GL_FOG_DENSITY, stage->fog->exp_density());
        } break;
        case FOG_TYPE_LINEAR:
        default: {
            GLCheck(glFogi, GL_FOG_MODE, GL_LINEAR);
            GLCheck(glFogf, GL_FOG_START, stage->fog->linear_start());
            GLCheck(glFogf, GL_FOG_END, stage->fog->linear_end());
        } break;
        }

        GLCheck(glFogfv, GL_FOG_COLOR, &stage->fog->colour().r);
    }
}

void GL1RenderQueueVisitor::visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration iteration) {
    do_visit(renderable, pass, iteration);
}

void GL1RenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(stage);
}

void GL1RenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

_S_FORCE_INLINE bool bind_texture(const GLubyte which, const TextureUnit* unit) {
    auto tex = unit->texture();
    auto id = (tex) ? tex->_renderer_specific_id() : 0;

    if(!id) {
        return false;
    }

    if(which >= _S_GL_MAX_TEXTURE_UNITS) {
        return false;
    }

#if _S_GL_SUPPORTS_MULTITEXTURE
    GLCheck(glActiveTexture, GL_TEXTURE0 + which);
#endif

    GLCheck(glBindTexture, GL_TEXTURE_2D, id);

    GLCheck(glMatrixMode, GL_TEXTURE);
    GLCheck(glLoadMatrixf, unit->texture_matrix().data());

    return true;
}

void GL1RenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    pass_ = next;

    if(!prev || prev->diffuse() != next->diffuse()) {
        auto diffuse = next->diffuse();
        GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_DIFFUSE, &diffuse.r);
    }

    if(!prev || prev->ambient() != next->ambient()) {
        auto ambient = next->ambient();
        GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_AMBIENT, &ambient.r);
    }

    if(!prev || prev->emission() != next->emission()) {
        auto emission = next->emission();
        GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_EMISSION, &emission.r);
    }

    if(!prev || prev->specular() != next->specular()) {
        auto specular = next->specular();
        GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r);
    }

    if(!prev || prev->shininess() != next->shininess()) {
        GLCheck(glMaterialf, GL_FRONT_AND_BACK, GL_SHININESS, next->shininess());
    }

    if(!prev || prev->is_depth_test_enabled() != next->is_depth_test_enabled()) {
        if(next->is_depth_test_enabled()) {
            GLCheck(glEnable, GL_DEPTH_TEST);
        } else {
            GLCheck(glDisable, GL_DEPTH_TEST);
        }
    }

    if(!prev || prev->is_depth_write_enabled() != next->is_depth_write_enabled()) {
        if(next->is_depth_write_enabled()) {
            GLCheck(glDepthMask, GL_TRUE);
        } else {
            GLCheck(glDepthMask, GL_FALSE);
        }
    }

    /* Enable lighting on the pass appropriately */
    if(!prev || prev->is_lighting_enabled() != next->is_lighting_enabled()) {
        if(next->is_lighting_enabled()) {
            GLCheck(glEnable, GL_LIGHTING);
        } else {
            GLCheck(glDisable, GL_LIGHTING);
        }
    }

    if(!prev || prev->is_texturing_enabled() != next->is_texturing_enabled()) {
        if(next->is_texturing_enabled()) {
            for(uint32_t i = 0; i < _S_GL_MAX_TEXTURE_UNITS; ++i) {
#if _S_GL_SUPPORTS_MULTITEXTURE
                GLCheck(glActiveTexture, GL_TEXTURE0 + i);
#endif
                GLCheck(glEnable, GL_TEXTURE_2D);
            }
        } else {
            for(uint32_t i = 0; i < _S_GL_MAX_TEXTURE_UNITS; ++i) {
#if _S_GL_SUPPORTS_MULTITEXTURE
                GLCheck(glActiveTexture, GL_TEXTURE0 + i);
#endif
                GLCheck(glDisable, GL_TEXTURE_2D);
            }
        }
    }

#if !defined(__DREAMCAST__) && !defined(__PSP__)
    if(!prev || prev->point_size() != next->point_size()) {
        glPointSize(next->point_size());
    }

    if(!prev || prev->polygon_mode() != next->polygon_mode()) {
        switch((PolygonMode) next->polygon_mode()) {
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

    if(!prev || prev->blend_func() != next->blend_func()) {
        set_blending_mode(next->blend_func());
    }

    if(!prev || prev->shade_model() != next->shade_model()) {
        if(next->shade_model() == SHADE_MODEL_SMOOTH) {
            GLCheck(glShadeModel, GL_SMOOTH);
        } else {
            GLCheck(glShadeModel, GL_FLAT);
        }
    }

#if _S_GL_SUPPORTS_COLOR_MATERIAL
    if(!prev || prev->colour_material() != next->colour_material()) {
        if(next->colour_material() == COLOUR_MATERIAL_NONE) {
            GLCheck(glDisable, GL_COLOR_MATERIAL);
        } else {
            switch(next->colour_material()) {
            case COLOUR_MATERIAL_AMBIENT:
                GLCheck(glColorMaterial, GL_FRONT_AND_BACK, GL_AMBIENT);
            break;
            case COLOUR_MATERIAL_DIFFUSE:
                GLCheck(glColorMaterial, GL_FRONT_AND_BACK, GL_DIFFUSE);
            break;
            case COLOUR_MATERIAL_AMBIENT_AND_DIFFUSE:
                GLCheck(glColorMaterial, GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            break;
            default:
                break;
            }

            GLCheck(glEnable, GL_COLOR_MATERIAL);
        }
    }
#endif

    if(pass_->is_texturing_enabled()) {
        uint8_t used_count = 0;

        if(bind_texture(used_count, pass_->diffuse_map())) {
            used_count++;
        }

        if(bind_texture(used_count, pass_->light_map())) {
            used_count++;
        }

        if(bind_texture(used_count, pass_->normal_map())) {
            used_count++;
        }

        if(bind_texture(used_count, pass_->specular_map())) {
            used_count++;
        }

        for(auto i = used_count; i < _S_GL_MAX_TEXTURE_UNITS; ++i) {
#if _S_GL_SUPPORTS_MULTITEXTURE
            GLCheck(glActiveTexture, GL_TEXTURE0 + i);
#endif
            GLCheck(glBindTexture, GL_TEXTURE_2D, 0);
        }
    }
}

void GL1RenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    if(!count) {
        return;
    }

    Light* current = nullptr;

    const LightState disabled_state;

    bool matrix_loaded = false;

    for(uint8_t i = 0; i < MAX_LIGHTS_PER_RENDERABLE; ++i) {
        current = (i < count) ? lights[i] : nullptr;

        auto state = (current) ? LightState(
            true,
            Vec4(current->absolute_position(), (current->type() == LIGHT_TYPE_DIRECTIONAL) ? 0 : 1),
            current->diffuse(),
            current->ambient(),
            current->specular(),
            current->constant_attenuation(),
            current->linear_attenuation(),
            current->quadratic_attenuation()
        ) : disabled_state;

        /* No need to update this light */
        if(light_states_[i].initialized && light_states_[i] == state) {
            continue;
        }

        if(state.enabled) {
            GLCheck(glEnable, GL_LIGHT0 + i);

            /* Only load the matrix on the first enabled light */
            if(!matrix_loaded) {
                GLCheck(glMatrixMode, GL_MODELVIEW);
                GLCheck(glPushMatrix);

                const Mat4& view = camera_->view_matrix();

                GLCheck(glLoadMatrixf, view.data());
                matrix_loaded = true;
            }

            GLCheck(glLightfv, GL_LIGHT0 + i, GL_POSITION, &state.position.x);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_AMBIENT, &state.ambient.r);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_DIFFUSE, &state.diffuse.r);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_SPECULAR, &state.specular.r);
            GLCheck(glLightf, GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, state.constant_att);
            GLCheck(glLightf, GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, state.linear_att);
            GLCheck(glLightf, GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, state.quadratic_att);

        } else {
            GLCheck(glDisable, GL_LIGHT0 + i);
        }

        light_states_[i] = state;
        light_states_[i].initialized = true;
    }

    if(matrix_loaded) {
        GLCheck(glPopMatrix);
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

void GL1RenderQueueVisitor::enable_normal_arrays(bool force) {
    if(!force && normals_enabled_) return;
    GLCheck(glEnableClientState, GL_NORMAL_ARRAY);
    normals_enabled_ = true;
}

void GL1RenderQueueVisitor::disable_normal_arrays(bool force) {
    if(!force && !normals_enabled_) return;

    GLCheck(glDisableClientState, GL_NORMAL_ARRAY);
    normals_enabled_ = false;
}

void GL1RenderQueueVisitor::enable_texcoord_array(uint8_t which, bool force) {
    assert(which < _S_GL_MAX_TEXTURE_UNITS);

    if(!force && textures_enabled_[which]) return;

#if _S_GL_SUPPORTS_MULTITEXTURE
    GLCheck(glClientActiveTexture, GL_TEXTURE0 + which);
#endif

    GLCheck(glEnableClientState, GL_TEXTURE_COORD_ARRAY);

    textures_enabled_[which] = true;
}

void GL1RenderQueueVisitor::disable_texcoord_array(uint8_t which, bool force) {
    assert(which < _S_GL_MAX_TEXTURE_UNITS);

    if(!force && !textures_enabled_[which]) {
        return;
    }

#if _S_GL_SUPPORTS_MULTITEXTURE
    GLCheck(glClientActiveTexture, GL_TEXTURE0 + which);
#endif

    GLCheck(glDisableClientState, GL_TEXTURE_COORD_ARRAY);
    textures_enabled_[which] = false;
}

static GLenum convert_arrangement(MeshArrangement arrangement) {
    switch(arrangement) {
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
    case MESH_ARRANGEMENT_QUADS:
        return GL_QUADS;
    default:
        assert(0 && "Invalid mesh arrangement");
        return GL_TRIANGLES;
    }
}

static GLenum convert_index_type(IndexType type) {
    switch(type) {
    case INDEX_TYPE_8_BIT: return GL_UNSIGNED_BYTE;
    case INDEX_TYPE_16_BIT: return GL_UNSIGNED_SHORT;
    case INDEX_TYPE_32_BIT: return GL_UNSIGNED_INT;
    default:
        assert(0 && "Invalid index type");
        return GL_UNSIGNED_SHORT;
    }
}

void GL1RenderQueueVisitor::do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    _S_UNUSED(material_pass);
    _S_UNUSED(iteration);

    auto element_count = renderable->index_element_count;
    // Don't bother doing *anything* if there is nothing to render
    if(!element_count) {
        return;
    }

    const Mat4 model = renderable->final_transformation;
    const Mat4& view = camera_->view_matrix();
    const Mat4& projection = camera_->projection_matrix();

    Mat4 modelview = view * model;

    GLCheck(glMatrixMode, GL_MODELVIEW);
    GLCheck(glLoadMatrixf, modelview.data());

    GLCheck(glMatrixMode, GL_PROJECTION);
    GLCheck(glLoadMatrixf, projection.data());

    const auto& spec = renderable->vertex_data->vertex_specification();
    const auto stride = spec.stride();

    renderer_->prepare_to_render(renderable);

    const auto vertex_data = renderable->vertex_data->data();
    const auto index_data = renderable->index_data->data();

    assert(vertex_data);
    assert(index_data);

    const auto has_positions = spec.has_positions();
    if(has_positions) {
        enable_vertex_arrays();
        GLCheck(
            glVertexPointer,
            (spec.position_attribute == VERTEX_ATTRIBUTE_2F) ? 2 : (spec.position_attribute == VERTEX_ATTRIBUTE_3F) ? 3 : 4,
            GL_FLOAT,
            stride,
            ((const uint8_t*) vertex_data) + spec.position_offset(false)
        );
    } else {
        disable_vertex_arrays();
    }

    const auto has_diffuse = spec.has_diffuse();
    if(has_diffuse) {
        enable_colour_arrays();
        GLCheck(
            glColorPointer,
            (spec.diffuse_attribute == VERTEX_ATTRIBUTE_2F) ? 2 :
            (spec.diffuse_attribute == VERTEX_ATTRIBUTE_3F) ? 3 :
            (spec.diffuse_attribute == VERTEX_ATTRIBUTE_4F) ? 4 : GL_BGRA, // This weirdness is an extension apparently
            (spec.diffuse_attribute == VERTEX_ATTRIBUTE_4UB) ? GL_UNSIGNED_BYTE : GL_FLOAT,
            stride,
            ((const uint8_t*) vertex_data) + spec.diffuse_offset(false)
        );
    } else {
        disable_colour_arrays();
    }

    const auto has_normals = spec.has_normals();
    if(has_normals) {
        enable_normal_arrays();

        auto type = (spec.normal_attribute == VERTEX_ATTRIBUTE_PACKED_VEC4_1I) ?
            GL_UNSIGNED_INT_2_10_10_10_REV : GL_FLOAT;

        /*
         * According to the ARB_vertex_type_2_10_10_10_rev extension, glNormalPointer
         * should be able to handle GL_UNSIGNED_INT_2_10_10_10_REV. However Mesa3D throws
         * a GL_INVALID_OPERATION if you attempt this (https://gitlab.freedesktop.org/mesa/mesa/issues/2111)
         *
         * So, don't try this on the desktop. The DEFAULT vertex specification only enables this on
         * the Dreamcast so we can hit the GLdc fast rendering path by matching the PVR vertex size (32 bytes)
         */

        GLCheck(
            glNormalPointer,
            type,
            stride,
            ((const uint8_t*) vertex_data) + spec.normal_offset(false)
        );
    } else {
        disable_normal_arrays();
    }

    for(uint8_t i = 0; i < _S_GL_MAX_TEXTURE_UNITS; ++i) {
        bool enabled = spec.has_texcoordX(i);

        if(enabled) {
            enable_texcoord_array(i);
            auto offset = spec.texcoordX_offset(i, false);

#if _S_GL_SUPPORTS_MULTITEXTURE
            GLCheck(glClientActiveTexture, GL_TEXTURE0 + i);
#endif
            GLCheck(
                glTexCoordPointer,
                (spec.texcoordX_attribute(i) == VERTEX_ATTRIBUTE_2F) ? 2 : (spec.texcoordX_attribute(i) == VERTEX_ATTRIBUTE_3F) ? 3 : 4,
                GL_FLOAT,
                stride,
                ((const uint8_t*) vertex_data) + offset
            );
        } else {
            disable_texcoord_array(i);
        }
    }

    assert(element_count);

    auto index_type = convert_index_type(renderable->index_data->index_type());
    auto arrangement = convert_arrangement(renderable->arrangement);

    GLCheck(
        glDrawElements,
        arrangement,
        element_count,
        index_type,
        (const void*) index_data
    );

    renderer_->window->stats->increment_polygons_rendered(renderable->arrangement, element_count);
}

}
