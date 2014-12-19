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

namespace kglt {

Pipeline::Pipeline(
        RenderSequence* render_sequence,
        PipelineID id):
    generic::Identifiable<PipelineID>(id),
    sequence_(render_sequence),
    priority_(0),
    is_active_(true) {

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
        pipeline(p)->activate();
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
        p->deactivate();
    }
}

PipelinePtr RenderSequence::pipeline(PipelineID pipeline) {
    return PipelineManager::manager_get(pipeline);
}

void RenderSequence::delete_pipeline(PipelineID pipeline_id) {
    PipelineManager::manager_delete(pipeline_id);
    ordered_pipelines_.remove_if([=](Pipeline::ptr pipeline) -> bool { return pipeline->id() == pipeline_id;});
}

void RenderSequence::delete_all_pipelines() {
    PipelineManager::manager_delete_all();
    ordered_pipelines_.clear();
}

PipelineID RenderSequence::new_pipeline(StageID stage, CameraID camera, ViewportID viewport, TextureID target, int32_t priority) {
    PipelineID new_p = PipelineManager::manager_new();

    ordered_pipelines_.push_back(PipelineManager::__objects()[new_p]);

    ordered_pipelines_.back()->set_stage(stage);
    ordered_pipelines_.back()->set_camera(camera);
    ordered_pipelines_.back()->set_viewport(viewport);
    ordered_pipelines_.back()->set_target(target);
    ordered_pipelines_.back()->set_priority(priority);

    ordered_pipelines_.sort(
        [](Pipeline::ptr lhs, Pipeline::ptr rhs) { return lhs->priority() < rhs->priority(); }
    );

    return new_p;
}

PipelineID RenderSequence::new_pipeline(UIStageID stage, CameraID camera, ViewportID viewport, TextureID target, int32_t priority) {
    PipelineID new_p = PipelineManager::manager_new();

    ordered_pipelines_.push_back(PipelineManager::__objects()[new_p]);

    ordered_pipelines_.back()->set_ui_stage(stage);
    ordered_pipelines_.back()->set_camera(camera);
    ordered_pipelines_.back()->set_viewport(viewport);
    ordered_pipelines_.back()->set_target(target);
    ordered_pipelines_.back()->set_priority(priority);

    ordered_pipelines_.sort(
        [](Pipeline::ptr lhs, Pipeline::ptr rhs) { return lhs->priority() < rhs->priority(); }
    );

    return new_p;
}

void RenderSequence::set_renderer(Renderer::ptr renderer) {
    renderer_ = renderer;
}

void RenderSequence::run() {
    window_.ViewportManager::apply_func_to_objects(std::bind(&Viewport::clear, std::placeholders::_1));

    for(Pipeline::ptr pipeline: ordered_pipelines_) {
        run_pipeline(pipeline);
    }
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

void RenderSequence::run_pipeline(Pipeline::ptr pipeline_stage) {
    if(!pipeline_stage->is_active()) {
        return;
    }

    update_camera_constraint(pipeline_stage->camera_id());

    Mat4 camera_projection = window_.camera(pipeline_stage->camera_id())->projection_matrix();

    auto viewport = window_.viewport(pipeline_stage->viewport_id());
    viewport->apply(); //FIXME apply shouldn't exist

    signal_pipeline_started_(*pipeline_stage);

    CameraID camera_id = pipeline_stage->camera_id();
    StageID stage_id = pipeline_stage->stage_id();

    if(pipeline_stage->ui_stage_id()) {        
        //This is a UI stage, so just render that
        auto ui_stage = window_.ui_stage(pipeline_stage->ui_stage_id());
        ui_stage->__resize(viewport->width(), viewport->height());
        ui_stage->__render(camera_projection);
    } else {
        std::vector<RenderablePtr> buffers = window_.stage(stage_id
            )->partitioner().geometry_visible_from(camera_id);


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

        //Go through the visible actors
        for(RenderablePtr ent: buffers) {
            //Get the priority queue for this actor (e.g. RENDER_PRIORITY_BACKGROUND)
            QueueGroups::mapped_type& priority_queue = queues[(uint32_t)ent->render_priority()];

            auto mat = window_.stage(pipeline_stage->stage_id())->material(ent->material_id());

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
                group->insert(*ent, pass);
            }
        }

        /*
         * At this point, we will have a render group tree for each priority level
         * when we render, we can apply the uniforms/textures/shaders etc. by traversing the
         * tree and calling bind()/unbind() at each level
         */
        renderer_->set_current_stage(stage_id);
        for(RenderPriority priority: RENDER_PRIORITIES) {
            QueueGroups::mapped_type& priority_queue = queues[priority];
            for(RootGroup::ptr pass_group: priority_queue) {
                std::function<void (Renderable&, MaterialPass&)> f = [=](Renderable& renderable, MaterialPass& pass) {
                    renderer_->render(
                        renderable,
                        pipeline_stage->camera_id(),
                        pass_group->get_root().current_program()
                    );
                };
                pass_group->traverse(f);
            }
        }
        renderer_->set_current_stage(StageID());
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
