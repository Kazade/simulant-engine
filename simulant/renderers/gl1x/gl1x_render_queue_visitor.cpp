
#include "simulant/types.h"
#ifdef __DREAMCAST__
#include "../../../deps/libgl/include/GL/gl.h"
#include "../../../deps/libgl/include/GL/glext.h"
#elif defined(__PSP__)
#include <GL/gl.h>
#else
    #include "../glad/glad/glad.h"

    #if defined(_MSC_VER) && defined(APIENTRY)
    #undef APIENTRY
    #endif

#endif

#include "gl1x_render_group_impl.h"
#include "gl1x_render_queue_visitor.h"
#include "gl1x_renderer.h"

#include "../../application.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../stage.h"
#include "../../utils/gl_error.h"
#include "../../window.h"

namespace smlt {

GL1RenderQueueVisitor::GL1RenderQueueVisitor(GL1XRenderer* renderer,
                                             CameraPtr camera) :
    renderer_(renderer), camera_(camera) {}

void GL1RenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue,
                                            uint64_t frame_id,
                                            StageNode* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);

    /* Set up default client state before the run. This is necessary
     * so that the boolean flags get correctly set */
    enable_vertex_arrays(true);
    enable_color_arrays(true);
    enable_normal_arrays(true);
    enable_texcoord_array(0, true);

    for(auto i = 1u; i < _S_GL_MAX_TEXTURE_UNITS; ++i) {
        disable_texcoord_array(i, true);
    }

    global_ambient_ = stage->scene->lighting->ambient_light();
    GLCheck(glLightModelfv, GL_LIGHT_MODEL_AMBIENT, &global_ambient_.r);
}

void GL1RenderQueueVisitor::visit(const Renderable* renderable,
                                  const MaterialPass* pass,
                                  batcher::Iteration iteration) {
    do_visit(renderable, pass, iteration);
}

void GL1RenderQueueVisitor::end_traversal(const batcher::RenderQueue& queue,
                                          StageNode* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(stage);
}

void GL1RenderQueueVisitor::change_render_group(
    const batcher::RenderGroup* prev, const batcher::RenderGroup* next) {
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

_S_FORCE_INLINE bool bind_texture(const GLubyte which, const TexturePtr& tex,
                                  const Mat4& mat) {
    if(!tex) {
        return false;
    }

    auto id = tex->_renderer_specific_id();

    if(which >= _S_GL_MAX_TEXTURE_UNITS) {
        return false;
    }

#if _S_GL_SUPPORTS_MULTITEXTURE
    GLCheck(glActiveTexture, GL_TEXTURE0 + which);
#endif

    GLCheck(glBindTexture, GL_TEXTURE_2D, id);
    GLCheck(glMatrixMode, GL_TEXTURE);
    GLCheck(glLoadMatrixf, mat.data());

    return true;
}

void GL1RenderQueueVisitor::change_material_pass(const MaterialPass* prev,
                                                 const MaterialPass* next) {
    pass_ = next;

    const auto& diffuse = next->diffuse();
    GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_DIFFUSE, &diffuse.r);

    const auto& ambient = next->ambient();
    GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_AMBIENT, &ambient.r);

    const auto& emission = next->emission();
    GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_EMISSION, &emission.r);

    const auto& specular = next->specular();
    GLCheck(glMaterialfv, GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r);
    GLCheck(glMaterialf, GL_FRONT_AND_BACK, GL_SHININESS, next->shininess());

    if(next->is_depth_test_enabled()) {
        GLCheck(glEnable, GL_DEPTH_TEST);
    } else {
        GLCheck(glDisable, GL_DEPTH_TEST);
    }

    if(next->is_depth_write_enabled()) {
        GLCheck(glDepthMask, GL_TRUE);
    } else {
        GLCheck(glDepthMask, GL_FALSE);
    }

    switch(next->depth_func()) {
        case DEPTH_FUNC_NEVER:
            GLCheck(glDepthFunc, GL_NEVER);
            break;
        case DEPTH_FUNC_LEQUAL:
            GLCheck(glDepthFunc, GL_LEQUAL);
            break;
        case DEPTH_FUNC_ALWAYS:
            GLCheck(glDepthFunc, GL_ALWAYS);
            break;
        case DEPTH_FUNC_EQUAL:
            GLCheck(glDepthFunc, GL_EQUAL);
            break;
        case DEPTH_FUNC_GEQUAL:
            GLCheck(glDepthFunc, GL_GEQUAL);
            break;
        case DEPTH_FUNC_GREATER:
            GLCheck(glDepthFunc, GL_GREATER);
            break;
        case DEPTH_FUNC_LESS:
            GLCheck(glDepthFunc, GL_LESS);
            break;
    }

    /* Enable lighting on the pass appropriately */
    if(next->is_lighting_enabled()) {
        GLCheck(glEnable, GL_LIGHTING);
    } else {
        GLCheck(glDisable, GL_LIGHTING);
    }

    auto enabled = next->textures_enabled();

#if !_S_GL_SUPPORTS_MULTITEXTURE
    auto glActiveTexture = [](GLenum) {};
#endif

#define CAT_I(a, b) a##b
#define CAT(a, b) CAT_I(a, b)

#define ENABLE_TEXTURE(i, map)                                                 \
    if(_S_GL_MAX_TEXTURE_UNITS > (i)) {                                        \
        if(enabled & (1 << (i))) {                                             \
            GLCheck(glActiveTexture, GL_TEXTURE0 + (i));                       \
            GLCheck(glEnable, GL_TEXTURE_2D);                                  \
            bind_texture((i), next->CAT(map, _map)(),                          \
                         next->CAT(map, _map_matrix)());                       \
        } else {                                                               \
            GLCheck(glActiveTexture, GL_TEXTURE0 + (i));                       \
            GLCheck(glBindTexture, GL_TEXTURE_2D, 0);                          \
            GLCheck(glDisable, GL_TEXTURE_2D);                                 \
        }                                                                      \
    }

    ENABLE_TEXTURE(0, diffuse);
    ENABLE_TEXTURE(1, light);
    ENABLE_TEXTURE(2, normal);
    ENABLE_TEXTURE(3, specular);

#if !defined(__DREAMCAST__) && !defined(__PSP__)
    if(!prev || prev->point_size() != next->point_size()) {
        glPointSize(next->point_size());
    }

    switch((PolygonMode)next->polygon_mode()) {
        case POLYGON_MODE_POINT:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case POLYGON_MODE_LINE:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
#endif

    switch(next->cull_mode()) {
        case CULL_MODE_NONE:
            glDisable(GL_CULL_FACE);
            break;
        case CULL_MODE_FRONT_FACE:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case CULL_MODE_BACK_FACE:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case CULL_MODE_FRONT_AND_BACK_FACE:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
            break;
    }

    GLCheck(glEnable, GL_ALPHA_TEST);
    auto alpha_ref = next->alpha_threshold();
    switch(next->alpha_func()) {
        case ALPHA_FUNC_NONE:
            GLCheck(glDisable, GL_ALPHA_TEST);
            break;
        case ALPHA_FUNC_EQUAL:
            GLCheck(glAlphaFunc, GL_EQUAL, alpha_ref);
            break;
        case ALPHA_FUNC_GEQUAL:
            GLCheck(glAlphaFunc, GL_GEQUAL, alpha_ref);
            break;
        case ALPHA_FUNC_GREATER:
            GLCheck(glAlphaFunc, GL_GREATER, alpha_ref);
            break;
        case ALPHA_FUNC_LEQUAL:
            GLCheck(glAlphaFunc, GL_LEQUAL, alpha_ref);
            break;
        case ALPHA_FUNC_LESS:
            GLCheck(glAlphaFunc, GL_LESS, alpha_ref);
            break;
        default:
            break;
    }

    GLCheck(glEnable, GL_BLEND);
    switch(next->blend_func()) {
        case BLEND_NONE:
            GLCheck(glDisable, GL_BLEND);
            break;
        case BLEND_ADD:
            GLCheck(glBlendFunc, GL_ONE, GL_ONE);
            break;
        case BLEND_ALPHA:
            GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_COLOR:
            GLCheck(glBlendFunc, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
            break;
        case BLEND_MODULATE:
            GLCheck(glBlendFunc, GL_DST_COLOR, GL_ZERO);
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
            GLCheck(glBlendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        default:
            break;
    }

    if(next->shade_model() == SHADE_MODEL_SMOOTH) {
        GLCheck(glShadeModel, GL_SMOOTH);
    } else {
        GLCheck(glShadeModel, GL_FLAT);
    }

#if _S_GL_SUPPORTS_COLOR_MATERIAL
    switch(next->color_material()) {
        case COLOR_MATERIAL_NONE:
            GLCheck(glDisable, GL_COLOR_MATERIAL);
            break;
        case COLOR_MATERIAL_AMBIENT:
            GLCheck(glEnable, GL_COLOR_MATERIAL);
            GLCheck(glColorMaterial, GL_FRONT_AND_BACK, GL_AMBIENT);
            break;
        case COLOR_MATERIAL_DIFFUSE:
            GLCheck(glEnable, GL_COLOR_MATERIAL);
            GLCheck(glColorMaterial, GL_FRONT_AND_BACK, GL_DIFFUSE);
            break;
        case COLOR_MATERIAL_AMBIENT_AND_DIFFUSE:
            GLCheck(glEnable, GL_COLOR_MATERIAL);
            GLCheck(glColorMaterial, GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            break;
        default:
            break;
    }
#endif

    auto next_mode = next->fog_mode();

    switch(next_mode) {
        case FOG_MODE_NONE: {
            GLCheck(glDisable, GL_FOG);
        } break;
        case FOG_MODE_EXP: {
            GLCheck(glEnable, GL_FOG);
            GLCheck(glFogi, GL_FOG_MODE, GL_EXP);
            GLCheck(glFogf, GL_FOG_DENSITY, next->fog_density());
            GLCheck(glFogfv, GL_FOG_COLOR, &next->fog_color().r);
        } break;
        case FOG_MODE_EXP2: {
            GLCheck(glEnable, GL_FOG);
            GLCheck(glFogi, GL_FOG_MODE, GL_EXP2);
            GLCheck(glFogf, GL_FOG_DENSITY, next->fog_density());
            GLCheck(glFogfv, GL_FOG_COLOR, &next->fog_color().r);
        } break;
        case FOG_MODE_LINEAR:
        default: {
            GLCheck(glEnable, GL_FOG);
            GLCheck(glFogi, GL_FOG_MODE, GL_LINEAR);
            GLCheck(glFogf, GL_FOG_START, next->fog_start());
            GLCheck(glFogf, GL_FOG_END, next->fog_end());
            GLCheck(glFogfv, GL_FOG_COLOR, &next->fog_color().r);
        } break;
    }
}

void GL1RenderQueueVisitor::apply_lights(const LightPtr* lights,
                                         const uint8_t count) {

    const LightState disabled_state;

    Light* current = nullptr;

    if(count) {
        GLCheck(glMatrixMode, GL_MODELVIEW);
        GLCheck(glPushMatrix);

        const Mat4& view = camera_->view_matrix();

        GLCheck(glLoadMatrixf, view.data());        
    }
    
    for(uint8_t i = 0; i < MAX_LIGHTS_PER_RENDERABLE; ++i) {
        current = (i < count) ? lights[i] : nullptr;

        smlt::Vec3 pos;
        LightState state = disabled_state;

        if(current) {
            pos = (current->light_type()) == LIGHT_TYPE_DIRECTIONAL
                      ? current->direction()
                      : current->transform->position();

            state = LightState(
                true,
                Vec4(pos,
                     (current->light_type() == LIGHT_TYPE_DIRECTIONAL) ? 0 : 1),
                current->diffuse(), current->ambient(), current->specular(),
                current->constant_attenuation(), current->linear_attenuation(),
                current->quadratic_attenuation());
        }

        /* No need to update this light */
        if(light_states_[i].initialized && light_states_[i] == state) {
            continue;
        }

        if(state.enabled) {
            GLCheck(glEnable, GL_LIGHT0 + i);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_POSITION, &state.position.x);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_AMBIENT, &state.ambient.r);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_DIFFUSE, &state.diffuse.r);
            GLCheck(glLightfv, GL_LIGHT0 + i, GL_SPECULAR, &state.specular.r);
            GLCheck(glLightf, GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,
                    state.constant_att);
            GLCheck(glLightf, GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,
                    state.linear_att);
            GLCheck(glLightf, GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION,
                    state.quadratic_att);

        } else {
            GLCheck(glDisable, GL_LIGHT0 + i);
        }

        light_states_[i] = state;
        light_states_[i].initialized = true;
    }

    if(count) {
        GLCheck(glPopMatrix);
    }
}

void GL1RenderQueueVisitor::enable_vertex_arrays(bool force) {
    if(!force && positions_enabled_) {
        return;
    }
    GLCheck(glEnableClientState, GL_VERTEX_ARRAY);
    positions_enabled_ = true;
}

void GL1RenderQueueVisitor::disable_vertex_arrays(bool force) {
    if(!force && !positions_enabled_) {
        return;
    }

    GLCheck(glDisableClientState, GL_VERTEX_ARRAY);
    positions_enabled_ = false;
}

void GL1RenderQueueVisitor::enable_color_arrays(bool force) {
    if(!force && colors_enabled_) {
        return;
    }
    GLCheck(glEnableClientState, GL_COLOR_ARRAY);
    colors_enabled_ = true;
}

void GL1RenderQueueVisitor::disable_color_arrays(bool force) {
    if(!force && !colors_enabled_) {
        return;
    }

    GLCheck(glDisableClientState, GL_COLOR_ARRAY);
    colors_enabled_ = false;
}

void GL1RenderQueueVisitor::enable_normal_arrays(bool force) {
    if(!force && normals_enabled_) {
        return;
    }
    GLCheck(glEnableClientState, GL_NORMAL_ARRAY);
    normals_enabled_ = true;
}

void GL1RenderQueueVisitor::disable_normal_arrays(bool force) {
    if(!force && !normals_enabled_) {
        return;
    }

    GLCheck(glDisableClientState, GL_NORMAL_ARRAY);
    normals_enabled_ = false;
}

void GL1RenderQueueVisitor::enable_texcoord_array(uint8_t which, bool force) {
    assert(which < _S_GL_MAX_TEXTURE_UNITS);

    if(!force && textures_enabled_[which]) {
        return;
    }

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

static constexpr GLenum convert_arrangement(MeshArrangement arrangement) {
    return (arrangement == MESH_ARRANGEMENT_LINES)        ? GL_LINES
           : (arrangement == MESH_ARRANGEMENT_LINE_STRIP) ? GL_LINE_STRIP
           : (arrangement == MESH_ARRANGEMENT_TRIANGLES)  ? GL_TRIANGLES
           : (arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP)
               ? GL_TRIANGLE_STRIP
           : (arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) ? GL_TRIANGLE_STRIP
           : (arrangement == MESH_ARRANGEMENT_QUADS)        ? GL_QUADS
                                                            : GL_TRIANGLES;
}

static constexpr GLenum convert_index_type(IndexType type) {
    return (type == INDEX_TYPE_8_BIT)    ? GL_UNSIGNED_BYTE
           : (type == INDEX_TYPE_16_BIT) ? GL_UNSIGNED_SHORT
                                         : GL_UNSIGNED_INT;
}

void GL1RenderQueueVisitor::do_visit(const Renderable* renderable,
                                     const MaterialPass* material_pass,
                                     batcher::Iteration iteration) {
    _S_UNUSED(material_pass);
    _S_UNUSED(iteration);

    auto element_count = renderable->index_element_count;
    auto vertex_range_count = renderable->vertex_range_count;
    // Don't bother doing *anything* if there is nothing to render
    if(!element_count && !vertex_range_count) {
        return;
    }

    const Mat4& model = renderable->final_transformation;
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
    assert(vertex_data);

    const auto has_positions = spec.has_positions();
    if(has_positions) {
        enable_vertex_arrays();
        GLCheck(glVertexPointer,
                (spec.position_attribute == VERTEX_ATTRIBUTE_2F)   ? 2
                : (spec.position_attribute == VERTEX_ATTRIBUTE_3F) ? 3
                                                                   : 4,
                GL_FLOAT, stride,
                ((const uint8_t*)vertex_data) + spec.position_offset(false));
    } else {
        disable_vertex_arrays();
    }

    const auto has_diffuse = spec.has_diffuse();
    if(has_diffuse) {
        S_VERBOSE("Enabling colors");
        enable_color_arrays();
        GLCheck(glColorPointer,
                (spec.diffuse_attribute == VERTEX_ATTRIBUTE_2F)   ? 2
                : (spec.diffuse_attribute == VERTEX_ATTRIBUTE_3F) ? 3
                : (spec.diffuse_attribute == VERTEX_ATTRIBUTE_4F ||
                   spec.diffuse_attribute == VERTEX_ATTRIBUTE_4UB_RGBA)
                    ? 4
                    : GL_BGRA, // This weirdness is an extension apparently
                (spec.diffuse_attribute == VERTEX_ATTRIBUTE_4UB_RGBA ||
                 spec.diffuse_attribute == VERTEX_ATTRIBUTE_4UB_BGRA)
                    ? GL_UNSIGNED_BYTE
                    : GL_FLOAT,
                stride,
                ((const uint8_t*)vertex_data) + spec.diffuse_offset(false));
    } else {
        disable_color_arrays();
    }

    const auto has_normals = spec.has_normals();
    if(has_normals) {
        enable_normal_arrays();

        auto type = (spec.normal_attribute == VERTEX_ATTRIBUTE_PACKED_VEC4_1I)
                        ? GL_UNSIGNED_INT_2_10_10_10_REV
                        : GL_FLOAT;

        /*
         * According to the ARB_vertex_type_2_10_10_10_rev extension,
         * glNormalPointer should be able to handle
         * GL_UNSIGNED_INT_2_10_10_10_REV. However Mesa3D throws a
         * GL_INVALID_OPERATION if you attempt this
         * (https://gitlab.freedesktop.org/mesa/mesa/issues/2111)
         *
         * So, don't try this on the desktop. The DEFAULT vertex specification
         * only enables this on the Dreamcast so we can hit the GLdc fast
         * rendering path by matching the PVR vertex size (32 bytes)
         */

        GLCheck(glNormalPointer, type, stride,
                ((const uint8_t*)vertex_data) + spec.normal_offset(false));
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
            GLCheck(glTexCoordPointer,
                    (spec.texcoordX_attribute(i) == VERTEX_ATTRIBUTE_2F)   ? 2
                    : (spec.texcoordX_attribute(i) == VERTEX_ATTRIBUTE_3F) ? 3
                                                                           : 4,
                    GL_FLOAT, stride, ((const uint8_t*)vertex_data) + offset);
        } else {
            disable_texcoord_array(i);
        }
    }

    auto arrangement = convert_arrangement(renderable->arrangement);

    if(element_count) {
        /* Indexed renderable */
        const auto index_data = renderable->index_data->data();
        auto index_type =
            convert_index_type(renderable->index_data->index_type());

        GLCheck(glDrawElements, arrangement, element_count, index_type,
                (const void*)index_data);

        get_app()->stats->increment_polygons_rendered(renderable->arrangement,
                                                      element_count);
    } else {
        /* Range-based renderable */
        assert(renderable->vertex_ranges);
        assert(renderable->vertex_range_count);

        auto range = renderable->vertex_ranges;
        auto total = 0;
        for(std::size_t i = 0; i < renderable->vertex_range_count;
            ++i, ++range) {
            GLCheck(glDrawArrays, arrangement, range->start, range->count);

            total += range->count;
        }

        get_app()->stats->increment_polygons_rendered(renderable->arrangement,
                                                      total);
    }
}

} // namespace smlt
