#include <tr1/unordered_map>

#include "pipeline.h"
#include "scene.h"

#include "mesh.h"
#include "light.h"

#include "window_base.h"
#include "partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "renderers/generic_renderer.h"
#include "batcher.h"

namespace kglt {

Stage::Stage(Scene& scene, SubSceneID ss, CameraID camera, ViewportID viewport, TextureID target):
    priority_(0),
    scene_(scene),
    subscene_(ss),
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

void Pipeline::add_stage(SubSceneID subscene, CameraID camera, ViewportID viewport, TextureID target, int32_t priority) {
    stages_.push_back(Stage::ptr(new Stage(scene_, subscene, camera, viewport, target)));
    stages_.at(stages_.size() - 1)->set_priority(priority);
}

void Pipeline::set_renderer(Renderer::ptr renderer) {
    renderer_ = renderer;
}

void Pipeline::run() {
    scene_.window().apply_func_to_objects(std::bind(&Viewport::clear, std::tr1::placeholders::_1));

    std::vector<Stage::ptr> sorted(stages_.begin(), stages_.end());
    std::sort(stages_.begin(), stages_.end(), [](Stage::ptr lhs, Stage::ptr rhs) { return lhs->priority() < rhs->priority(); });

    for(Stage::ptr stage: sorted) {
        run_stage(stage);
    }
}

void Pipeline::run_stage(Stage::ptr stage) {
    scene_.window().viewport(stage->viewport_id()).apply(); //FIXME apply shouldn't exist

    signal_render_stage_started_(*stage);

    SubScene& subscene = scene_.subscene(stage->subscene_id());
    std::vector<SubEntity::ptr> buffers = subscene.partitioner().geometry_visible_from(stage->camera_id());


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

        Material& mat = subscene.material(ent->material_id());
        //Go through the entities material passes
        for(uint8_t pass = 0; pass < mat.technique().pass_count(); ++pass) {
            //Create a new render group if necessary
            RootGroup::ptr group;
            if(priority_queue.size() <= pass) {
                group = RootGroup::ptr(new RootGroup(subscene));
                priority_queue.push_back(group);
            } else {
                group = priority_queue[pass];
            }

            //Insert the entity into the RenderGroup tree
            group->insert(*ent, pass);
        }
    }

    renderer_->set_current_subscene(subscene.id());
    for(RenderPriority priority: RENDER_PRIORITIES) {
        QueueGroups::mapped_type& priority_queue = queues[priority];
        for(RootGroup::ptr pass_group: priority_queue) {
            pass_group->traverse(std::tr1::bind(&Renderer::render_subentity, renderer_.get(), std::tr1::placeholders::_1, stage->camera_id()));
        }
    }
    renderer_->set_current_subscene(SubSceneID());

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
    renderer_->set_current_subscene(subscene.id());
        renderer_->render(buffers, stage->camera_id());
    renderer_->set_current_subscene(SubSceneID());*/

    signal_render_stage_finished_(*stage);
}

}
