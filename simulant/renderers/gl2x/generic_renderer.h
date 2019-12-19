/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>
#include <memory>
#include <cstdint>
#include "../renderer.h"
#include "../gl_renderer.h"
#include "../../material.h"
#include "../batching/render_queue.h"

namespace smlt {

class GL2RenderGroupImpl;
class GenericRenderer;
class VBOManager;
class GPUBuffer;

struct RenderState {
    Renderable* renderable;
    MaterialPass* pass;
    const Light* light;
    batcher::Iteration iteration;
    GL2RenderGroupImpl* render_group_impl;
};

class GL2RenderQueueVisitor : public batcher::RenderQueueVisitor {
public:
    GL2RenderQueueVisitor(GenericRenderer* renderer, CameraPtr camera);

    void start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage);
    void visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration);
    void end_traversal(const batcher::RenderQueue &queue, Stage* stage);

    void change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next);
    void change_material_pass(const MaterialPass* prev, const MaterialPass* next);
    void apply_lights(const LightPtr* lights, const uint8_t count);

private:
    GenericRenderer* renderer_;
    CameraPtr camera_;
    Colour global_ambient_;

    GPUProgram* program_ = nullptr;
    const MaterialPass* pass_ = nullptr;
    const Light* light_ = nullptr;

    GL2RenderGroupImpl* current_group_ = nullptr;

    void do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration);

    void rebind_attribute_locations_if_necessary(const MaterialPass* pass, GPUProgram* program);
};

typedef ObjectManager<GPUProgramID, GPUProgram, DO_REFCOUNT> GPUProgramManager;

class GenericRenderer:
    public Renderer,
    private GLRenderer {

public:
    GenericRenderer(Window* window);

    batcher::RenderGroupKey prepare_render_group(
        batcher::RenderGroup* group,
        const Renderable *renderable,
        const MaterialPass *material_pass,
        const uint8_t pass_number,
        const bool is_blended,
        const float distance_to_camera
    ) override;

    void init_context() override;

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera) override;

    GPUProgramID new_or_existing_gpu_program(const std::string& vertex_shader_source, const std::string& fragment_shader_source) override;

    GPUProgramPtr gpu_program(const GPUProgramID& program_id) const override;
    GPUProgramID current_gpu_program_id() const override;
    bool supports_gpu_programs() const override { return true; }
    GPUProgramID default_gpu_program_id() const override;

    std::string name() const override {
        return "gl2x";
    }

    void prepare_to_render(const Renderable* renderable) override;
private:
    GPUProgramManager program_manager_;
    GPUProgramID default_gpu_program_id_;

    std::shared_ptr<VBOManager> buffer_manager_;

    void set_light_uniforms(const MaterialPass* pass, GPUProgram* program, const LightPtr light);
    void set_material_uniforms(const MaterialPass *pass, GPUProgram* program);
    void set_renderable_uniforms(const MaterialPass* pass, GPUProgram* program, const Renderable* renderable, Camera* camera);
    void set_stage_uniforms(const MaterialPass* pass, GPUProgram* program, const Colour& global_ambient);

    void set_auto_attributes_on_shader(GPUProgram *program, const Renderable* buffer, GPUBuffer* buffers);
    void set_blending_mode(BlendType type);
    void send_geometry(const Renderable* renderable, GPUBuffer* buffers);

    /* Stashed here in prepare_to_render and used later for that renderable */
    std::shared_ptr<GPUBuffer> buffer_stash_;

    friend class GL2RenderQueueVisitor;

    void on_texture_prepare(TexturePtr texture) override {
        GLRenderer::on_texture_prepare(texture);
    }

    void on_texture_register(TextureID tex_id, TexturePtr texture) override {
        GLRenderer::on_texture_register(tex_id, texture);
    }

    void on_texture_unregister(TextureID tex_id, Texture* texture) override {
        GLRenderer::on_texture_unregister(tex_id, texture);
    }
};

}

#endif // GENERIC_RENDERER_H
