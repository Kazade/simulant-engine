#include <tr1/unordered_map>

#include "pipeline.h"
#include "scene.h"
#include "stage.h"
#include "entity.h"
#include "mesh.h"
#include "light.h"

#include "window_base.h"
#include "partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "renderers/generic_renderer.h"
#include "batcher.h"
#include "loader.h"

namespace kglt {

PipelineStage::PipelineStage(Scene& scene, StageID ss, CameraID camera, ViewportID viewport, TextureID target):
    priority_(0),
    scene_(scene),
    stage_(ss),
    target_(target),
    camera_(camera),
    viewport_(viewport) {
}

Pipeline::Pipeline(Scene& scene):
    scene_(scene),
    renderer_(new GenericRenderer(scene)) {

    //Set up the default render options
    render_options.wireframe_enabled = false;
    render_options.texture_enabled = true;
    render_options.backface_culling_enabled = true;
    render_options.point_size = 1;
}

void Pipeline::remove_all_stages() {
    stages_.clear();
}

void Pipeline::add_stage(StageID stage, CameraID camera, ViewportID viewport, TextureID target, int32_t priority) {
    stages_.push_back(PipelineStage::ptr(new PipelineStage(scene_, stage, camera, viewport, target)));
    stages_.at(stages_.size() - 1)->set_priority(priority);
}

void Pipeline::set_renderer(Renderer::ptr renderer) {
    renderer_ = renderer;
}

void Pipeline::run() {
    scene_.window().apply_func_to_objects(std::bind(&Viewport::clear, std::tr1::placeholders::_1));

    std::vector<PipelineStage::ptr> sorted(stages_.begin(), stages_.end());
    std::sort(stages_.begin(), stages_.end(), [](PipelineStage::ptr lhs, PipelineStage::ptr rhs) { return lhs->priority() < rhs->priority(); });

    for(PipelineStage::ptr stage: sorted) {
        run_stage(stage);
    }
}

void Pipeline::run_stage(PipelineStage::ptr pipeline_stage) {
    scene_.window().viewport(pipeline_stage->viewport_id()).apply(); //FIXME apply shouldn't exist

    signal_render_stage_started_(*pipeline_stage);

    Stage& stage = scene_.stage(pipeline_stage->stage_id());
    Camera& camera = stage.camera(pipeline_stage->camera_id());

    std::vector<SubEntity::ptr> buffers = stage.partitioner().geometry_visible_from(pipeline_stage->camera_id());


    /*
     * Go through the visible objects, sort into queues and for
     * each material pass add the subentity. The result is that a tree
     * of material properties (uniforms) will be created with the child nodes
     * being the meshes
     */
    typedef std::tr1::unordered_map<uint32_t, std::vector<RootGroup::ptr> > QueueGroups;
    QueueGroups queues;

    //Go through the visible entities
    for(SubEntity::ptr ent: buffers) {
        //Get the priority queue for this entity (e.g. RENDER_PRIORITY_BACKGROUND)
        QueueGroups::mapped_type& priority_queue = queues[(uint32_t)ent->_parent().render_priority()];

        MaterialPtr mat = stage.material(ent->material_id()).lock();
        //Go through the entities material passes
        for(uint8_t pass = 0; pass < mat->technique().pass_count(); ++pass) {
            //Create a new render group if necessary
            RootGroup::ptr group;
            if(priority_queue.size() <= pass) {
                group = RootGroup::ptr(new RootGroup(stage, camera));
                priority_queue.push_back(group);
            } else {
                group = priority_queue[pass];
            }

            //Insert the entity into the RenderGroup tree
            group->insert(*ent, pass);
        }
    }

    renderer_->set_current_stage(stage.id());
    for(RenderPriority priority: RENDER_PRIORITIES) {
        QueueGroups::mapped_type& priority_queue = queues[priority];
        for(RootGroup::ptr pass_group: priority_queue) {
            std::function<void (SubEntity&)> f = [=](SubEntity& subentity) {
                renderer_->render_subentity(subentity, pipeline_stage->camera_id());
            };
            pass_group->traverse(f);
        }
    }
    renderer_->set_current_stage(StageID());

    /*
     * At this point, we will have a render group tree for each priority level
     * when we render, we can apply the uniforms/textures/shaders etc. by traversing the
     * tree and calling bind()/unbind() at each level
     */

/*
    std::sort(buffers.begin(), buffers.end(), [](SubEntity::ptr lhs, SubEntity::ptr rhs) {
        return lhs->_parent().render_priority() < rhs->_parent().render_priority();
    });

    //TODO: Batched rendering
    renderer_->set_current_stage(stage.id());
        renderer_->render(buffers, stage->camera_id());
    renderer_->set_current_stage(StageID());*/

    signal_render_stage_finished_(*pipeline_stage);
}

}
