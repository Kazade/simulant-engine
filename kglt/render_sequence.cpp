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
#include "renderers/generic_renderer.h"
#include "batcher.h"
#include "loader.h"
#include "gpu_program.h"

namespace kglt {

Pipeline::Pipeline(
        RenderSequence* render_sequence,
        PipelineID id):
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
        sequence_->window_.stage(stage_)->decrement_render_count();
    } else if(ui_stage_) {
        sequence_->window_.ui_stage(ui_stage_)->decrement_render_count();
    }
}

void Pipeline::activate() {
    if(is_active_) return;

    is_active_ = true;

    if(stage_) {
        sequence_->window_.stage(stage_)->increment_render_count();
    } else if(ui_stage_) {
        sequence_->window_.ui_stage(ui_stage_)->increment_render_count();
    }
}

RenderSequence::RenderSequence(WindowBase &window):
    window_(window),
    renderer_(new GenericRenderer(window)) {

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
    PipelineID new_p = PipelineManager::manager_new();

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
    PipelineID new_p = PipelineManager::manager_new();

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

void RenderSequence::set_renderer(Renderer::ptr renderer) {
    renderer_ = renderer;
}

void RenderSequence::run() {
    targets_rendered_this_frame_.clear();

    int actors_rendered = 0;
    for(Pipeline::ptr pipeline: ordered_pipelines_) {
        run_pipeline(pipeline, actors_rendered);
    }

    window_.stats->set_subactors_rendered(actors_rendered);
}

void RenderSequence::update_camera_constraint(CameraID cid) {
    auto camera = window_.camera(cid);

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

const uint32_t RENDER_PRIORITY_RANGE = RENDER_PRIORITY_MAX - RENDER_PRIORITY_ABSOLUTE_BACKGROUND;
const uint32_t MAX_MATERIAL_PASSES = 25;

struct RenderQueue {
    RootGroup* passes_[MAX_MATERIAL_PASSES] = {nullptr};

    RootGroup* get_or_create_pass(uint32_t pass, WindowBase* window, StageID stage_id, CameraID camera_id) {
        assert(pass < MAX_MATERIAL_PASSES);
        if(!passes_[pass]) {
            passes_[pass] = new RootGroup(*window, stage_id, camera_id);
        }
        return passes_[pass];
    }

    ~RenderQueue() {
        for(uint32_t i = 0; i < MAX_MATERIAL_PASSES; ++i) {
            if(passes_[i]) {
                delete passes_[i];
                passes_[i] = nullptr;
            }
        }
    }

    void each(std::function<void(RootGroup*)> func) {
        for(RootGroup* group: passes_) {
            if(group) {
                func(group);
            }
        }
    }
};

struct QueuesByPriority {
    /*
     * This is a set of render queues indexed by their render priority
     * Everything is raw pointers and static arrays for performance.
     *
     * It might be a wise idea to just allocate the entire range on construction
     * but this is fast enough for now
     */
    RenderQueue* queues_[RENDER_PRIORITY_RANGE] = {nullptr};

    RenderQueue* get_or_create_queue(RenderPriority priority) {
        uint32_t idx = uint32_t(priority - RENDER_PRIORITY_ABSOLUTE_BACKGROUND);

        assert(idx < RENDER_PRIORITY_RANGE);

        if(!queues_[idx]) {
            queues_[idx] = new RenderQueue();
        }

        return queues_[idx];
    }

    void clear() {
        for(uint32_t i = 0; i < RENDER_PRIORITY_RANGE; ++i) {
            if(queues_[i]) {
                delete queues_[i];
                queues_[i] = nullptr;
            }
        }
    }

    ~QueuesByPriority() {
        clear();
    }

    void each(std::function<void(RenderQueue*)> func) {
        for(RenderQueue* queue: queues_) {
            if(queue) {
                func(queue);
            }
        }
    }
};

void RenderSequence::run_pipeline(Pipeline::ptr pipeline_stage, int &actors_rendered) {
    static QueuesByPriority queues;
    //Empty the queues each call, but retain the same container (saves allocations)
    queues.clear();

    if(!pipeline_stage->is_active()) {
        return;
    }

    update_camera_constraint(pipeline_stage->camera_id());

    Mat4 camera_projection = window_.camera(pipeline_stage->camera_id())->projection_matrix();

    RenderTarget& target = window_; //FIXME: Should be window or texture

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

    auto viewport = pipeline_stage->viewport();

    uint32_t clear = pipeline_stage->clear_flags();
    if(clear) {
        viewport.clear(target, clear); //Implicitly calls apply
    } else {
        viewport.apply(target); //FIXME apply shouldn't exist, it ties Viewport to OpenGL...
    }

    signal_pipeline_started_(*pipeline_stage);

    CameraID camera_id = pipeline_stage->camera_id();
    StageID stage_id = pipeline_stage->stage_id();

    if(pipeline_stage->ui_stage_id()) {        
        //This is a UI stage, so just render that
        auto ui_stage = window_.ui_stage(pipeline_stage->ui_stage_id());
        ui_stage->__resize(viewport.width_in_pixels(target), viewport.height_in_pixels(target));
        ui_stage->__render(camera_projection);
    } else {
        auto stage = window_.stage(stage_id);

        std::vector<RenderablePtr> buffers = stage->partitioner().geometry_visible_from(camera_id);
        std::vector<LightID> lights = stage->partitioner().lights_visible_from(camera_id);


        /*
         * Go through the visible objects, sort into queues and for
         * each material pass add the subactor. The result is that a tree
         * of material properties (uniforms) will be created with the child nodes
         * being the meshes
         */
        std::vector<LightID> lights_intersecting_actor;

        //Go through the visible actors
        for(RenderablePtr ent: buffers) {
            //Get the priority queue for this actor (e.g. RENDER_PRIORITY_BACKGROUND)
            RenderQueue* priority_queue = queues.get_or_create_queue(ent->render_priority());
            assert(priority_queue);

            auto mat = stage->material(ent->material_id());

            lights_intersecting_actor.clear();
            for(auto lid: lights) {
                auto light = stage->light(lid);
                if(light->type() == LIGHT_TYPE_DIRECTIONAL || light->transformed_aabb().intersects(ent->transformed_aabb())) {
                    lights_intersecting_actor.push_back(lid);
                }
            }

            //Go through the actor's material passes building up the render tree
            mat->each([&](uint32_t i, MaterialPass* pass) {
                RootGroup* group = priority_queue->get_or_create_pass(i, &window_, stage_id, camera_id);
                assert(group);
                group->insert(ent.get(), pass, lights_intersecting_actor);
            });

            actors_rendered++;
        }

        /*
         * At this point, we will have a render group tree for each priority level
         * when we render, we can apply the uniforms/textures/shaders etc. by traversing the
         * tree and calling bind()/unbind() at each level
         */
        renderer_->set_current_stage(stage_id);

        queues.each([=](RenderQueue* queue) {
            assert(queue);

            queue->each([=](RootGroup* pass_group) {
                assert(pass_group);

                auto callback = [&](Renderable* renderable, MaterialPass* pass) {
                    renderer_->render(
                        *renderable,
                        pipeline_stage->camera_id(),
                        pass->program.get()
                    );
                };

                pass_group->traverse(callback);
            });
        });

        renderer_->set_current_stage(StageID());
    }

    signal_pipeline_finished_(*pipeline_stage);
}

}
