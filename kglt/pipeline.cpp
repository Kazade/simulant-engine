#include "pipeline.h"
#include "scene.h"

#include "mesh.h"
#include "light.h"

#include "window_base.h"
#include "partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "renderers/generic_renderer.h"

namespace kglt {

Pass::Pass(Scene& scene, SceneGroupID sg, TextureID target, CameraID camera, ViewportID viewport):
    scene_(scene),
    scene_group_(sg),
    target_(target),
    camera_(camera),
    viewport_(viewport) {
}

Pipeline::Pipeline(Scene& scene):
    scene_(scene),
    partitioner_(new OctreePartitioner(scene)),
    renderer_(new GenericRenderer(scene)) {

    //Set up the default render options
    render_options.wireframe_enabled = false;
    render_options.texture_enabled = true;
    render_options.backface_culling_enabled = true;
    render_options.point_size = 1;
}

void Pipeline::init() {
    //Keep the partitioner updated with new meshes
    scene_.signal_entity_created().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::add_entity));
    scene_.signal_entity_destroyed().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::remove_entity));

    scene_.signal_light_created().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::add_light));
    scene_.signal_light_destroyed().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::remove_light));
}

void Pipeline::remove_all_passes() {
    passes_.clear();
}

void Pipeline::add_pass(SceneGroupID scene_group, TextureID target, CameraID camera, ViewportID viewport) {
    passes_.push_back(Pass::ptr(new Pass(scene_, scene_group, target, camera, viewport)));
}

void Pipeline::set_partitioner(Partitioner::ptr partitioner) {
    partitioner_ = partitioner;
}

void Pipeline::set_renderer(Renderer::ptr renderer) {
    renderer_ = renderer;
}

void Pipeline::run() {    
    for(uint32_t i = 0; i < passes_.size(); ++i) {       
        run_pass(i);
    }
}

void Pipeline::run_pass(uint32_t index) {
    Pass::ptr pass = passes_.at(index);

    scene_.window().viewport(pass->viewport()).update_opengl(); //FIXME update_opengl shouldn't exist

    signal_render_pass_started_(index);

    std::vector<SubEntity::ptr> buffers = partitioner_->geometry_visible_from(pass->camera_, pass->scene_group_);

    //TODO: Batched rendering
    renderer_->render(buffers, pass->camera());

    signal_render_pass_finished_(index);
}

}
