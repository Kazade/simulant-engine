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

void Pipeline::add_stage(SubSceneID subscene, CameraID camera, ViewportID viewport, TextureID target) {
    stages_.push_back(Stage::ptr(new Stage(scene_, subscene, camera, viewport, target)));
}

void Pipeline::set_renderer(Renderer::ptr renderer) {
    renderer_ = renderer;
}

void Pipeline::run() {
    scene_.window().apply_func_to_objects(std::bind(&Viewport::clear, std::tr1::placeholders::_1));
    for(uint32_t i = 0; i < stages_.size(); ++i) {
        run_stage(i);
    }
}

void Pipeline::run_stage(uint32_t index) {
    Stage::ptr stage = stages_.at(index);

    scene_.window().viewport(stage->viewport_id()).apply(); //FIXME apply shouldn't exist

    signal_render_stage_started_(index);

    std::vector<SubEntity::ptr> buffers = scene_.subscene(stage->subscene_id()).partitioner().geometry_visible_from(stage->camera_id());

    //TODO: Batched rendering
    renderer_->set_current_subscene(stage->subscene_id());
        renderer_->render(buffers, stage->camera_id());
    renderer_->set_current_subscene(SubSceneID());

    signal_render_stage_finished_(index);
}

}
