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
#include "lua/console.h"

namespace kglt {

Pipeline::Pipeline(
        RenderSequence* render_sequence,
        PipelineID id):
    generic::Identifiable<PipelineID>(id),
    sequence_(render_sequence),
    priority_(0),
    is_active_(false) {

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
    window_(window) {

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

    PipelineManager::manager_delete(pipeline_id);
    ordered_pipelines_.remove_if([=](Pipeline::ptr pipeline) -> bool { return pipeline->id() == pipeline_id;});
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

PipelineID RenderSequence::new_pipeline(StageID stage, CameraID camera, const Viewport& viewport, TextureID target, int32_t priority) {
    PipelineID new_p = PipelineManager::manager_new();

    ordered_pipelines_.push_back(PipelineManager::__objects()[new_p]);

    ordered_pipelines_.back()->set_stage(stage);
    ordered_pipelines_.back()->set_camera(camera);
    ordered_pipelines_.back()->set_viewport(viewport);
    ordered_pipelines_.back()->set_target(target);
    ordered_pipelines_.back()->set_priority(priority);
    ordered_pipelines_.back()->activate();

    ordered_pipelines_.sort(
        [](Pipeline::ptr lhs, Pipeline::ptr rhs) { return lhs->priority() < rhs->priority(); }
    );

    return new_p;
}

PipelineID RenderSequence::new_pipeline(UIStageID stage, CameraID camera, const Viewport& viewport, TextureID target, int32_t priority) {
    PipelineID new_p = PipelineManager::manager_new();

    ordered_pipelines_.push_back(PipelineManager::__objects()[new_p]);

    ordered_pipelines_.back()->set_ui_stage(stage);
    ordered_pipelines_.back()->set_camera(camera);
    ordered_pipelines_.back()->set_viewport(viewport);
    ordered_pipelines_.back()->set_target(target);
    ordered_pipelines_.back()->set_priority(priority);
    ordered_pipelines_.back()->activate();

    ordered_pipelines_.sort(
        [](Pipeline::ptr lhs, Pipeline::ptr rhs) { return lhs->priority() < rhs->priority(); }
    );

    return new_p;
}

void RenderSequence::run() {
    targets_rendered_this_frame_.clear();

    int actors_rendered = 0;
    for(Pipeline::ptr pipeline: ordered_pipelines_) {
        run_pipeline(pipeline, actors_rendered);
    }

    window_.console->set_stats_subactors_rendered(actors_rendered);
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

void RenderSequence::run_pipeline(Pipeline::ptr pipeline_stage, int &actors_rendered) {
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
        typedef std::unordered_map<int32_t, std::vector<RootGroup::ptr> > QueueGroups;
        static QueueGroups queues;

        //Empty the queues
        for(auto& queue: queues) {
            for(auto& group: queue.second) {
                group->clear();
            }
            queue.second.clear();
        }

        std::vector<LightID> lights_intersecting_actor;

        //Go through the visible actors
        for(RenderablePtr ent: buffers) {
            //Get the priority queue for this actor (e.g. RENDER_PRIORITY_BACKGROUND)
            QueueGroups::mapped_type& priority_queue = queues[(uint32_t)ent->render_priority()];

            auto mat = stage->material(ent->material_id());

            lights_intersecting_actor.clear();
            for(auto lid: lights) {
                auto light = stage->light(lid);
                if(light->type() == LIGHT_TYPE_DIRECTIONAL || light->transformed_aabb().intersects(ent->transformed_aabb())) {
                    lights_intersecting_actor.push_back(lid);
                }
            }

            //Go through the actors material passes
            for(uint8_t pass = 0; pass < mat->pass_count(); ++pass) {
                //Create a new render group if necessary
                RootGroup::ptr group;
                if(priority_queue.size() <= pass) {
                    group = RootGroup::ptr(new RootGroup(window_, stage_id, camera_id));
                    priority_queue.push_back(group);
                } else {
                    group = priority_queue[pass];
                }

                //Insert the actor into the RenderGroup tree
                group->insert(*ent, pass, lights_intersecting_actor);
            }

            actors_rendered++;
        }

        /*
         * At this point, we will have a render group tree for each priority level
         * when we render, we can apply the uniforms/textures/shaders etc. by traversing the
         * tree and calling bind()/unbind() at each level
         */
        window_.renderer->set_current_stage(stage_id);
        for(RenderPriority priority: RENDER_PRIORITIES) {
            QueueGroups::mapped_type& priority_queue = queues[priority];
            for(RootGroup::ptr pass_group: priority_queue) {
                std::function<void (Renderable&, MaterialPass&)> f = [=](Renderable& renderable, MaterialPass& pass) {
                    window_.renderer->render(
                        renderable,
                        pipeline_stage->camera_id(),
                        pass_group->get_root().current_program()
                    );
                };
                pass_group->traverse(f);
            }
        }
        window_.renderer->set_current_stage(StageID());
    }

/*
    std::sort(buffers.begin(), buffers.end(), [](SubActor::ptr lhs, SubActor::ptr rhs) {
        return lhs->_parent().render_priority() < rhs->_parent().render_priority();
    });

    //TODO: Batched rendering
    renderer_->set_current_stage(stage.id());
        renderer_->render(buffers, stage->camera_id());
    renderer_->set_current_stage(StageID());*/

    signal_pipeline_finished_(*pipeline_stage);
}

}
