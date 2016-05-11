#include "utils/glcompat.h"
#include <unordered_map>

#include "utils/gl_error.h"
#include "render_sequence.h"
#include "stage.h"
#include "ui_stage.h"
#include "actor.h"
#include "mesh.h"
#include "light.h"
#include "camera.h"
#include "window_base.h"
#include "partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "renderers/gl2x/generic_renderer.h"
#include "loader.h"
#include "gpu_program.h"
#include "renderers/render_queue.h"

namespace kglt {

Pipeline::Pipeline(PipelineID id,
        RenderSequence* render_sequence):
    generic::Identifiable<PipelineID>(id),
    sequence_(render_sequence),
    priority_(0),
    is_active_(false) {

}

void Pipeline::set_priority(int32_t priority) {
    if(priority_ != priority) {
        priority_ = priority;

        /* If the priority changed, we need to update the render sequence */
        sequence_->sort_pipelines(/*acquire_lock=*/true);
    }
}

Pipeline::~Pipeline() {
    deactivate();
}

void Pipeline::deactivate() {
    if(!is_active_) return;

    is_active_ = false;

    if(stage_) {
        sequence_->window->stage(stage_)->decrement_render_count();
    } else if(ui_stage_) {
        sequence_->window->ui_stage(ui_stage_)->decrement_render_count();
    }
}

void Pipeline::activate() {
    if(is_active_) return;

    is_active_ = true;

    if(stage_) {
        sequence_->window->stage(stage_)->increment_render_count();
    } else if(ui_stage_) {
        sequence_->window->ui_stage(ui_stage_)->increment_render_count();
    }
}

RenderSequence::RenderSequence(WindowBase *window):
    window_(window),
    renderer_(window->renderer.get()) {

    //Set up the default render options
    render_options.wireframe_enabled = false;
    render_options.texture_enabled = true;
    render_options.backface_culling_enabled = true;
    render_options.point_size = 1;
}

void RenderSequence::activate_pipelines(const std::vector<PipelineID>& pipelines) {
    for(PipelineID p: pipelines) {
        auto pip = pipeline(p);
        if(!pip->is_active()) {
            pip->activate();
        }
    }
}

std::vector<PipelineID> RenderSequence::active_pipelines() const {
    std::vector<PipelineID> result;

    for(Pipeline::ptr p: ordered_pipelines_) {
        if(p->is_active()) {
            result.push_back(p->id());
        }
    }

    return result;
}

void RenderSequence::deactivate_all_pipelines() {
    for(Pipeline::ptr p: ordered_pipelines_) {
        if(p->is_active()) {
            p->deactivate();
        }
    }
}

PipelinePtr RenderSequence::pipeline(PipelineID pipeline) {
    return PipelineManager::manager_get(pipeline);
}

void RenderSequence::delete_pipeline(PipelineID pipeline_id) {
    if(!PipelineManager::manager_contains(pipeline_id)) {
        return;
    }

    auto pip = pipeline(pipeline_id);
    if(pip->is_active()) {
        pip->deactivate();
    }

    ordered_pipelines_.remove_if([=](Pipeline::ptr pipeline) -> bool { return pipeline->id() == pipeline_id;});
    PipelineManager::manager_delete(pipeline_id);    
}

void RenderSequence::delete_all_pipelines() {
    PipelineManager::manager_delete_all();

    for(auto pip: ordered_pipelines_) {
        if(pip->is_active()) {
            pip->deactivate();
        }
    }

    ordered_pipelines_.clear();
}

void RenderSequence::sort_pipelines(bool acquire_lock) {
    auto do_sort = [&]() {
        ordered_pipelines_.sort(
            [](Pipeline::ptr lhs, Pipeline::ptr rhs) { return lhs->priority() < rhs->priority(); }
        );
    };

    if(acquire_lock) {
        std::lock_guard<std::mutex> lock(pipeline_lock_);
        do_sort();
    } else {
        do_sort();
    }
}

PipelineID RenderSequence::new_pipeline(StageID stage, CameraID camera, const Viewport& viewport, TextureID target, int32_t priority) {
    PipelineID new_p = PipelineManager::manager_new(this);

    auto pipeline = PipelineManager::manager_get(new_p).lock();

    pipeline->set_stage(stage);
    pipeline->set_camera(camera);
    pipeline->set_viewport(viewport);
    pipeline->set_target(target);
    pipeline->set_priority(priority);
    pipeline->activate();

    std::lock_guard<std::mutex> lock(pipeline_lock_);
    ordered_pipelines_.push_back(pipeline);
    sort_pipelines();

    return new_p;
}

PipelineID RenderSequence::new_pipeline(UIStageID stage, CameraID camera, const Viewport& viewport, TextureID target, int32_t priority) {
    PipelineID new_p = PipelineManager::manager_new(this);

    ordered_pipelines_.push_back(PipelineManager::manager_get(new_p).lock());

    auto pipeline = PipelineManager::manager_get(new_p).lock();

    pipeline->set_ui_stage(stage);
    pipeline->set_camera(camera);
    pipeline->set_viewport(viewport);
    pipeline->set_target(target);
    pipeline->set_priority(priority);
    pipeline->activate();

    std::lock_guard<std::mutex> lock(pipeline_lock_);
    ordered_pipelines_.push_back(pipeline);
    sort_pipelines();

    return new_p;
}

void RenderSequence::set_renderer(Renderer* renderer) {
    renderer_ = renderer;
}

void RenderSequence::run() {
    targets_rendered_this_frame_.clear();

    int actors_rendered = 0;
    for(Pipeline::ptr pipeline: ordered_pipelines_) {
        run_pipeline(pipeline, actors_rendered);
    }

    window->stats->set_subactors_rendered(actors_rendered);
}

void RenderSequence::update_camera_constraint(CameraID cid) {
    auto camera = window->camera(cid);

    if(camera->has_proxy()) {
        //Update the associated camera
        if(camera->proxy().is_constrained()) {
            //FIXME: THis might work for cameras but we need a more generic place
            //to do this for all objects before render
            camera->proxy()._update_constraint();
        }
        camera->set_transform(camera->proxy().absolute_transformation());
    }
}

uint64_t generate_frame_id() {
    static uint64_t frame_id = 0;
    return ++frame_id;
}

void RenderSequence::run_pipeline(Pipeline::ptr pipeline_stage, int &actors_rendered) {
    uint64_t frame_id = generate_frame_id();

    if(!pipeline_stage->is_active()) {
        return;
    }

    update_camera_constraint(pipeline_stage->camera_id());

    Mat4 camera_projection = window->camera(pipeline_stage->camera_id())->projection_matrix();

    RenderTarget& target = *window_; //FIXME: Should be window or texture

    /*
     *  Render targets can specify whether their buffer should be cleared at the start of each frame. We do this the first
     *  time we hit a render target when processing the pipelines. We keep track of the targets that have been rendered each frame
     *  and this list is cleared at the start of run().
     */
    if(targets_rendered_this_frame_.find(&target) == targets_rendered_this_frame_.end()) {
        if(target.clear_every_frame_flags()) {
            Viewport view(kglt::VIEWPORT_TYPE_FULL, target.clear_every_frame_colour());
            view.clear(target, target.clear_every_frame_flags());
        }

        targets_rendered_this_frame_.insert(&target);
    }

    auto& viewport = pipeline_stage->viewport;

    uint32_t clear = pipeline_stage->clear_flags();
    if(clear) {
        viewport->clear(target, clear); //Implicitly calls apply
    } else {
        viewport->apply(target); //FIXME apply shouldn't exist, it ties Viewport to OpenGL...
    }

    signal_pipeline_started_(*pipeline_stage);

    CameraID camera_id = pipeline_stage->camera_id();
    StageID stage_id = pipeline_stage->stage_id();

    if(pipeline_stage->ui_stage_id()) {        
        //This is a UI stage, so just render that
        auto ui_stage = window->ui_stage(pipeline_stage->ui_stage_id());
        ui_stage->__resize(viewport->width_in_pixels(target), viewport->height_in_pixels(target));
        ui_stage->__render(camera_projection);
    } else {
        auto stage = window->stage(stage_id);

        // Mark the visible objects as visible
        for(auto& renderable: stage->partitioner().geometry_visible_from(camera_id)) {
            renderable->update_last_visible_frame_id(frame_id);
        }

        using namespace std::placeholders;

        new_batcher::RenderQueue::TraverseCallback callback = std::bind(&Renderer::render, renderer_, _1, _2, _3, _4, _5);
        // Render the visible objects
        stage->render_queue->traverse(callback, frame_id);
    }

    signal_pipeline_finished_(*pipeline_stage);
}

}
