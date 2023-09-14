#pragma once

#include "../../assets/material.h"
#include "../batching/renderable.h"
#include "../gl_renderer.h"

namespace smlt {

class GL1RenderGroupImpl;
class GL1XRenderer;

struct GL1RenderState {
    Renderable* renderable;
    MaterialPass* pass;
    LightPtr light;
    batcher::Iteration iteration;
    GL1RenderGroupImpl* render_group_impl;
};


class GL1RenderQueueVisitor : public batcher::RenderQueueVisitor {
public:
    GL1RenderQueueVisitor(GL1XRenderer* renderer, CameraPtr camera);

    void start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, StageNode *stage);
    void visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration);
    void end_traversal(const batcher::RenderQueue &queue, StageNode* stage);

    void change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next);
    void change_material_pass(const MaterialPass* prev, const MaterialPass* next);
    void apply_lights(const LightPtr* lights, const uint8_t count);

private:
    GL1XRenderer* renderer_;
    CameraPtr camera_;
    Colour global_ambient_;

    const MaterialPass* pass_ = nullptr;
    LightPtr light_;

    GL1RenderGroupImpl* current_group_ = nullptr;

    void do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration);

    void enable_vertex_arrays(bool force=false);
    void disable_vertex_arrays(bool force=false);

    void enable_normal_arrays(bool force=false);
    void disable_normal_arrays(bool force=false);

    void enable_colour_arrays(bool force=false);
    void disable_colour_arrays(bool force=false);

    void enable_texcoord_array(uint8_t which, bool force=false);
    void disable_texcoord_array(uint8_t which, bool force=false);

    bool positions_enabled_ = false;
    bool colours_enabled_ = false;
    bool normals_enabled_ = false;
    bool textures_enabled_[_S_GL_MAX_TEXTURE_UNITS] = {0};

    uint32_t default_texture_name_ = 0;

    struct LightState {
        bool initialized = false;
        bool enabled = false;
        Vec4 position;
        Colour diffuse;
        Colour ambient;
        Colour specular;
        float constant_att = 0;
        float linear_att = 0;
        float quadratic_att = 0;

        LightState() = default;
        LightState(bool enabled, Vec4 pos, Colour diffuse, Colour ambient, Colour specular, float constant_att, float linear_att, float quadratic_att):
            enabled(enabled),
            position(pos),
            diffuse(diffuse),
            ambient(ambient),
            specular(specular),
            constant_att(constant_att),
            linear_att(linear_att),
            quadratic_att(quadratic_att) {}

        bool operator!=(const LightState& rhs) const {
            return !(*this == rhs);
        }

        bool operator==(const LightState& rhs) const {
            return (
                enabled == rhs.enabled &&
                position == rhs.position &&
                diffuse == rhs.diffuse &&
                ambient == rhs.ambient &&
                specular == rhs.specular &&
                constant_att == rhs.constant_att &&
                linear_att == rhs.linear_att &&
                quadratic_att == rhs.quadratic_att
            );
        }
    };

    LightState light_states_[MAX_LIGHTS_PER_RENDERABLE];
};


}
