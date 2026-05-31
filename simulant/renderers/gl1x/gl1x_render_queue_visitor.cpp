
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
#include "../../meshes/submesh.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../stage.h"
#include "../../utils/gl_error.h"
#include "../../utils/vertex_lighting.h"
#include "../../vertex_data.h"
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

    /* Upload the projection matrix once per traversal since it's constant
     * for all renderables rendered with this camera. This avoids redundant
     * glMatrixMode/glLoadMatrixf calls in do_visit(). */
    GLCheck(glMatrixMode, GL_PROJECTION);
    GLCheck(glLoadMatrixf, camera_->projection_matrix().data());
}

void GL1RenderQueueVisitor::visit(const Renderable* renderable,
                                  const MaterialPass* pass,
                                  batcher::Iteration iteration) {
    /* Modifier-volume passes are only meaningful on renderers that target a
     * non-default polygon list (e.g. the PVR). Skip them here. */
    if(pass->polygon_list_target() != POLYGON_LIST_TARGET_NONE) {
        return;
    }
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
    auto id = (tex) ? tex->_renderer_specific_id() : 0;

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

    /* Store PBR material properties for software per-vertex lighting */
    const Color& bc = next->base_color();
    mat_base_color_[0] = bc.r;
    mat_base_color_[1] = bc.g;
    mat_base_color_[2] = bc.b;
    mat_base_color_[3] = bc.a;
    mat_metallic_  = next->metallic();
    mat_roughness_ = next->roughness();
    mat_lighting_enabled_ = next->is_lighting_enabled();

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

    /* Software per-vertex PBR handles all lighting; keep GL lighting off */
    GLCheck(glDisable, GL_LIGHTING);

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

    ENABLE_TEXTURE(0, base_color);
    ENABLE_TEXTURE(1, light);
    ENABLE_TEXTURE(2, normal);
    ENABLE_TEXTURE(3, metallic_roughness);

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
#else
    _S_UNUSED(prev);
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

    switch(next->blend_func()) {
        case BLEND_NONE:
            GLCheck(glDisable, GL_BLEND);
            GLCheck(glDisable, GL_ALPHA_TEST);
            break;
        case BLEND_MASK:
            GLCheck(glDisable, GL_BLEND);
            GLCheck(glEnable, GL_ALPHA_TEST);
            GLCheck(glAlphaFunc, GL_GREATER, next->alpha_threshold());
            break;
        case BLEND_ADD:
            GLCheck(glDisable, GL_ALPHA_TEST);
            GLCheck(glEnable, GL_BLEND);
            GLCheck(glBlendFunc, GL_ONE, GL_ONE);
            break;
        case BLEND_ALPHA:
            GLCheck(glDisable, GL_ALPHA_TEST);
            GLCheck(glEnable, GL_BLEND);
            GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_COLOR:
            GLCheck(glDisable, GL_ALPHA_TEST);
            GLCheck(glEnable, GL_BLEND);
            GLCheck(glBlendFunc, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
            break;
        case BLEND_MODULATE:
            GLCheck(glDisable, GL_ALPHA_TEST);
            GLCheck(glEnable, GL_BLEND);
            GLCheck(glBlendFunc, GL_DST_COLOR, GL_ZERO);
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
            GLCheck(glDisable, GL_ALPHA_TEST);
            GLCheck(glEnable, GL_BLEND);
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
    /* Disable color-material: vertex colours are PBR-computed and used directly */
    GLCheck(glDisable, GL_COLOR_MATERIAL);
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

    /* Color write mask */
    if(next->is_color_write_enabled()) {
        GLCheck(glColorMask, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    } else {
        GLCheck(glColorMask, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

#if !defined(__PSP__) && !defined(__DREAMCAST__)
    /* Stencil test — not supported on PSP, and gldc on the Dreamcast doesn't
     * expose the stencil constants. The PVR has its own (modifier-volume)
     * shadow path, so stencil-shadow passes are skipped at the visitor level
     * on the Dreamcast. */
    if(next->is_stencil_test_enabled()) {
        GLCheck(glEnable, GL_STENCIL_TEST);

        static auto to_gl_func = [](StencilFunc f) -> GLenum {
            switch(f) {
                case STENCIL_FUNC_NEVER:     return GL_NEVER;
                case STENCIL_FUNC_LESS:      return GL_LESS;
                case STENCIL_FUNC_LEQUAL:    return GL_LEQUAL;
                case STENCIL_FUNC_GREATER:   return GL_GREATER;
                case STENCIL_FUNC_GEQUAL:    return GL_GEQUAL;
                case STENCIL_FUNC_EQUAL:     return GL_EQUAL;
                case STENCIL_FUNC_NOT_EQUAL: return GL_NOTEQUAL;
                case STENCIL_FUNC_ALWAYS:
                default:                     return GL_ALWAYS;
            }
        };
        static auto to_gl_op = [](StencilOp o) -> GLenum {
            switch(o) {
                case STENCIL_OP_ZERO:      return GL_ZERO;
                case STENCIL_OP_REPLACE:   return GL_REPLACE;
                case STENCIL_OP_INCR:      return GL_INCR;
                case STENCIL_OP_INCR_WRAP: return GL_INCR_WRAP;
                case STENCIL_OP_DECR:      return GL_DECR;
                case STENCIL_OP_DECR_WRAP: return GL_DECR_WRAP;
                case STENCIL_OP_INVERT:    return GL_INVERT;
                case STENCIL_OP_KEEP:
                default:                   return GL_KEEP;
            }
        };

        GLCheck(glStencilFunc, to_gl_func(next->stencil_func()),
                next->stencil_ref(), (GLuint)next->stencil_mask());
        GLCheck(glStencilOp,
                to_gl_op(next->stencil_fail_op()),
                to_gl_op(next->stencil_depth_fail_op()),
                to_gl_op(next->stencil_pass_op()));
    } else {
        GLCheck(glDisable, GL_STENCIL_TEST);
    }
#endif
}

void GL1RenderQueueVisitor::apply_lights(const LightPtr* lights,
                                         const uint8_t count) {
    vl_light_count_ = 0;
    for(uint8_t i = 0; i < MAX_LIGHTS_PER_RENDERABLE; ++i) {
        vl_lights_[i].enabled = false;
    }

    const Mat4& view = camera_->view_matrix();

    for(uint8_t i = 0; i < count && i < MAX_LIGHTS_PER_RENDERABLE; ++i) {
        if(!lights[i]) continue;

        auto light = lights[i];
        VertexLightState& state = vl_lights_[i];
        state.enabled = true;
        vl_light_count_ = i + 1;

        auto pos = view * light->transform->position();
        state.position[0] = pos.x;
        state.position[1] = pos.y;
        state.position[2] = pos.z;
        state.position[3] = (light->light_type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0f : 1.0f;

        state.color[0] = light->color().r;
        state.color[1] = light->color().g;
        state.color[2] = light->color().b;

        state.intensity = light->intensity();
        state.range = light->range();

        /* Pre-normalise toward-light direction for directional lights */
        if(state.position[3] < 0.5f) {
            float len = std::sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
            if(len > 1e-8f) {
                state.dir[0] = -pos.x / len;
                state.dir[1] = -pos.y / len;
                state.dir[2] = -pos.z / len;
            }
        }
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
           : (arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) ? GL_TRIANGLE_FAN
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

    const Mat4& model = *renderable->final_transformation;
    const Mat4& view = camera_->view_matrix();

    Mat4 modelview = view * model;

    GLCheck(glMatrixMode, GL_MODELVIEW);
    GLCheck(glLoadMatrixf, modelview.data());

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

    /* --- Software per-vertex PBR lighting ---
     * When lighting is enabled and normals are present, compute PBR colour
     * for every vertex into soft_color_buf_ and use that as the colour array.
     * When conditions aren't met, fall back to the raw vertex colour data. */
    const bool do_soft_lighting = mat_lighting_enabled_ && has_normals
                                  && vl_light_count_ > 0;
    const float global_amb[3] = {global_ambient_.r, global_ambient_.g, global_ambient_.b};

    if(do_soft_lighting) {
        uint32_t vcount = renderable->vertex_data->count();
        soft_color_buf_.resize(vcount * 4);

        auto pos_off    = spec.has_positions() ? spec.position_offset(false) : 0;
        auto normal_off = spec.normal_offset(false);
        auto color_off  = spec.has_color() ? spec.color_offset(false) : 0;

        for(uint32_t vi = 0; vi < vcount; ++vi) {
            const uint8_t* vp = ((const uint8_t*)vertex_data) + vi * stride;

            /* Read model-space position */
            const float* p_raw = (const float*)(vp + pos_off);
            float px = p_raw[0], py = p_raw[1], pz = (spec.position_attribute != VERTEX_ATTRIBUTE_2F) ? p_raw[2] : 0.0f;

            /* Transform to eye space */
            Vec4 ep = modelview * Vec4(px, py, pz, 1.0f);
            float eye_pos[3] = {ep.x, ep.y, ep.z};

            /* Read and transform normal (Mat4*Vec3 uses upper-3x3, no translation) */
            const float* n_raw = (const float*)(vp + normal_off);
            Vec3 nv = (modelview * Vec3(n_raw[0], n_raw[1], n_raw[2])).normalized();
            float N[3] = {nv.x, nv.y, nv.z};

            /* Effective base colour = material base_color modulated by vertex colour */
            float base_c[4] = {mat_base_color_[0], mat_base_color_[1],
                                mat_base_color_[2], mat_base_color_[3]};
            if(color_off) {
                const uint8_t* c = vp + color_off;
                float vc_r, vc_g, vc_b, vc_a;
                VertexAttribute attr = spec.color_attribute;
                if(attr == VERTEX_ATTRIBUTE_4F) {
                    const float* fc = (const float*)c;
                    vc_r=fc[0]; vc_g=fc[1]; vc_b=fc[2]; vc_a=fc[3];
                } else if(attr == VERTEX_ATTRIBUTE_3F) {
                    const float* fc = (const float*)c;
                    vc_r=fc[0]; vc_g=fc[1]; vc_b=fc[2]; vc_a=1.0f;
                } else if(attr == VERTEX_ATTRIBUTE_4UB_RGBA || attr == VERTEX_ATTRIBUTE_4UB) {
                    vc_r=c[0]/255.0f; vc_g=c[1]/255.0f; vc_b=c[2]/255.0f; vc_a=c[3]/255.0f;
                } else if(attr == VERTEX_ATTRIBUTE_4UB_BGRA) {
                    vc_b=c[0]/255.0f; vc_g=c[1]/255.0f; vc_r=c[2]/255.0f; vc_a=c[3]/255.0f;
                } else { vc_r=vc_g=vc_b=vc_a=1.0f; }
                base_c[0]*=vc_r; base_c[1]*=vc_g; base_c[2]*=vc_b; base_c[3]*=vc_a;
            }

            compute_pbr_vertex_color(N, eye_pos, base_c, mat_metallic_, mat_roughness_,
                                     global_amb, vl_lights_, vl_light_count_,
                                     &soft_color_buf_[vi * 4]);
        }

        enable_color_arrays();
        GLCheck(glColorPointer, 4, GL_FLOAT, 0, soft_color_buf_.data());
    } else {
        /* No software lighting — use vertex colour array or disable */
        const bool has_color = spec.has_color();
        if(has_color) {
            S_VERBOSE("Enabling colors");
            enable_color_arrays();
            GLCheck(glColorPointer,
                    (spec.color_attribute == VERTEX_ATTRIBUTE_2F)   ? 2
                    : (spec.color_attribute == VERTEX_ATTRIBUTE_3F) ? 3
                    : (spec.color_attribute == VERTEX_ATTRIBUTE_4F ||
                       spec.color_attribute == VERTEX_ATTRIBUTE_4UB_RGBA)
                        ? 4
                        : GL_BGRA,
                    (spec.color_attribute == VERTEX_ATTRIBUTE_4UB_RGBA ||
                     spec.color_attribute == VERTEX_ATTRIBUTE_4UB_BGRA)
                        ? GL_UNSIGNED_BYTE
                        : GL_FLOAT,
                    stride,
                    ((const uint8_t*)vertex_data) + spec.color_offset(false));
        } else {
            disable_color_arrays();
        }
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
