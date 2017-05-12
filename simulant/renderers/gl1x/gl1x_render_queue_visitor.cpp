
#include "gl1x_render_queue_visitor.h"
#include "gl1x_renderer.h"
#include "gl1x_render_group_impl.h"

#include "../../camera.h"

namespace smlt {


GL1RenderQueueVisitor::GL1RenderQueueVisitor(GL1XRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void GL1RenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage) {

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

}

void GL1RenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {

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

void GL1RenderQueueVisitor::do_visit(Renderable* renderable, MaterialPass* material_pass, batcher::Iteration iteration) {
    if(queue_if_blended(renderable, material_pass, iteration)) {
        // If this was a transparent object, and we were queuing then do nothing else for now
        return;
    }

    // Don't bother doing *anything* if there is nothing to render
    if(!renderable->index_element_count()) {
        return;
    }




}

}
