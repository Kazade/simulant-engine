#include <GLee.h>
#include <tr1/unordered_map>

#include "render_sequence.h"
#include "scene.h"
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

RenderSequence::RenderSequence(Scene& scene):
    scene_(scene),
    renderer_(new GenericRenderer(scene)) {

    //Set up the default render options
    render_options.wireframe_enabled = false;
    render_options.texture_enabled = true;
    render_options.backface_culling_enabled = true;
    render_options.point_size = 1;
}

void RenderSequence::activate_pipelines(const std::vector<PipelineID>& pipelines) {
    for(PipelineID p: pipelines) {
        pipeline(p).activate();
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

Pipeline& RenderSequence::pipeline(PipelineID pipeline) {
    return PipelineManager::manager_get(pipeline);
}

void RenderSequence::delete_pipeline(PipelineID pipeline) {
    PipelineManager::manager_delete(pipeline);
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
    scene_.window().apply_func_to_objects(std::bind(&Viewport::clear, std::tr1::placeholders::_1));

    for(Pipeline::ptr pipeline: ordered_pipelines_) {
        run_pipeline(pipeline);
    }
}

void RenderSequence::run_pipeline(Pipeline::ptr pipeline_stage) {
    if(!pipeline_stage->is_active()) {
        return;
    }

    Camera& camera = scene_.camera(pipeline_stage->camera_id());
    Viewport& viewport = scene_.window().viewport(pipeline_stage->viewport_id());
    viewport.apply(); //FIXME apply shouldn't exist

    signal_pipeline_started_(*pipeline_stage);

    if(pipeline_stage->ui_stage_id()) {
        //This is a UI stage, so just render that
        auto ui_stage = scene_.ui_stage(pipeline_stage->ui_stage_id());
        ui_stage->__resize(viewport.width(), viewport.height());

        //FIXME: GL 2.x rubbish
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(camera.projection_matrix().mat);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        ui_stage->__render();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    } else {
        Stage& stage = scene_.stage(pipeline_stage->stage_id());

        std::vector<SubActor::ptr> buffers = stage.partitioner().geometry_visible_from(pipeline_stage->camera_id());


        /*
         * Go through the visible objects, sort into queues and for
         * each material pass add the subactor. The result is that a tree
         * of material properties (uniforms) will be created with the child nodes
         * being the meshes
         */
        typedef std::tr1::unordered_map<uint32_t, std::vector<RootGroup::ptr> > QueueGroups;
        static QueueGroups queues;

        //Empty the queues
        for(auto queue: queues) {
            for(auto group: queue.second) {
                group->clear();
            }
        }

        //Go through the visible actors
        for(SubActor::ptr ent: buffers) {
            //Get the priority queue for this actor (e.g. RENDER_PRIORITY_BACKGROUND)
            QueueGroups::mapped_type& priority_queue = queues[(uint32_t)ent->_parent().render_priority()];

            auto mat = stage.material(ent->material_id());

            //Go through the actors material passes
            for(uint8_t pass = 0; pass < mat->technique().pass_count(); ++pass) {
                //Create a new render group if necessary
                RootGroup::ptr group;
                if(priority_queue.size() <= pass) {
                    group = RootGroup::ptr(new RootGroup(stage, camera));
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
        renderer_->set_current_stage(stage.id());
        for(RenderPriority priority: RENDER_PRIORITIES) {
            QueueGroups::mapped_type& priority_queue = queues[priority];
            for(RootGroup::ptr pass_group: priority_queue) {
                std::function<void (SubActor&)> f = [=](SubActor& subactor) {
                    renderer_->render_subactor(subactor, pipeline_stage->camera_id());
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
