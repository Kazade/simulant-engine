#include "pipeline.h"
#include "scene.h"

#include "mesh.h"
#include "light.h"

#include "window_base.h"
#include "partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "renderers/generic_renderer.h"

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

    //TODO: Batched rendering
    renderer_->set_current_subscene(subscene.id());
        renderer_->render(buffers, stage->camera_id());
    renderer_->set_current_subscene(SubSceneID());

    signal_render_stage_finished_(*stage);
}

}
