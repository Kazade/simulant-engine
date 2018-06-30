/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GENERIC_RENDERER_H
#define GENERIC_RENDERER_H

#include <vector>

#include "../renderer.h"
#include "../gl_renderer.h"
#include "../../material.h"
#include "./buffer_manager.h"
#include "../batching/render_queue.h"

namespace smlt {

class GL2RenderGroupImpl;
class GL3Renderer;

struct RenderState {
    Renderable* renderable;
    MaterialPass* pass;
    const Light* light;
    batcher::Iteration iteration;
    GL2RenderGroupImpl* render_group_impl;
};

class GL3RenderQueueVisitor : public batcher::RenderQueueVisitor {
public:
    GL3RenderQueueVisitor(GL3Renderer* renderer, CameraPtr camera);

    void start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage);
    void visit(Renderable* renderable, MaterialPass* pass, batcher::Iteration);
    void end_traversal(const batcher::RenderQueue &queue, Stage* stage);

    void change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next);
    void change_material_pass(const MaterialPass* prev, const MaterialPass* next);
    void change_light(const Light* prev, const Light* next);

private:
    GL3Renderer* renderer_;
    CameraPtr camera_;
    Colour global_ambient_;

    GPUProgram* program_ = nullptr;
    const MaterialPass* pass_ = nullptr;
    const Light* light_ = nullptr;

    GL2RenderGroupImpl* current_group_ = nullptr;

    bool queue_blended_objects_ = true;

    /*
     * All entries are ordered by distance from the near frustum descending (back-to-front)
     */
    std::multimap<float, RenderState, std::greater<float> > blended_object_queue_;

    void do_visit(Renderable* renderable, MaterialPass* material_pass, batcher::Iteration iteration);

    void rebind_attribute_locations_if_necessary(const MaterialPass* pass, GPUProgram* program);
};

typedef generic::RefCountedTemplatedManager<GPUProgram, GPUProgramID> GPUProgramManager;

class GL3Renderer:
    public Renderer,
    private GLRenderer {

public:
    GL3Renderer(Window* window):
        Renderer(window),
        GLRenderer(window),
        buffer_manager_(new GL2BufferManager(this)) {

    }

    batcher::RenderGroup new_render_group(Renderable *renderable, MaterialPass *material_pass);
    void init_context();

    std::shared_ptr<batcher::RenderQueueVisitor> get_render_queue_visitor(CameraPtr camera);

    GPUProgramID new_or_existing_gpu_program(const std::string& vertex_shader_source, const std::string& fragment_shader_source);

    GPUProgramPtr gpu_program(const GPUProgramID& program_id);

    bool supports_gpu_programs() const override { return true; }

private:
    GPUProgramManager program_manager_;

    std::unique_ptr<HardwareBufferManager> buffer_manager_;

    HardwareBufferManager* _get_buffer_manager() const {
        return buffer_manager_.get();
    }

    void set_light_uniforms(const MaterialPass* pass, GPUProgram* program, const Light *light);
    void set_material_uniforms(const MaterialPass *pass, GPUProgram* program);
    void set_renderable_uniforms(const MaterialPass* pass, GPUProgram* program, Renderable* renderable, Camera* camera);
    void set_stage_uniforms(const MaterialPass* pass, GPUProgram* program, const Colour& global_ambient);

    void prepare_vertex_array_object(const VertexSpecification& spec);
    void set_blending_mode(BlendType type);
    void send_geometry(Renderable* renderable);

    friend class GL3RenderQueueVisitor;

    void on_texture_prepare(TexturePtr texture) override {
        GLRenderer::on_texture_prepare(texture);
    }

    void on_texture_register(TextureID tex_id, TexturePtr texture) override {
        GLRenderer::on_texture_register(tex_id, texture);
    }

    void on_texture_unregister(TextureID tex_id) override {
        GLRenderer::on_texture_unregister(tex_id);
    }

    /* We create one VAO per vertex specification so we only have to set up vertex pointers once
     * for each spec type. At some point this all needs cleaning up and optimising but this'll do for now
     */
    std::unordered_map<VertexSpecification, GLuint> vertex_array_objects_;
};

}

#endif // GENERIC_RENDERER_H
