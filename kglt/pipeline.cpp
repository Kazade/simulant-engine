#include "pipeline.h"
#include "scene.h"

#include "mesh.h"
#include "light.h"

#include "partitioner.h"
#include "partitioners/null_partitioner.h"
#include "renderers/generic_renderer.h"

namespace kglt {

Pass::Pass(Scene& scene, SceneGroupID sg, TextureID target, CameraID camera, ViewportType viewport):
    scene_(scene),
    scene_group_(sg),
    target_(target),
    camera_(camera),
    viewport_(&scene) {

    viewport_.configure(viewport);

}

Pipeline::Pipeline(Scene& scene):
    scene_(scene),
    partitioner_(new NullPartitioner(scene)), //TODO: Should be Octree-powered GenericPartitioner
    renderer_(new GenericRenderer(scene)) {

    //Keep the partitioner updated with new meshes
    scene_.signal_mesh_created().connect(sigc::mem_fun<Mesh&>(partitioner_.get(), &kglt::Partitioner::add));
    scene_.signal_mesh_destroyed().connect(sigc::mem_fun<Mesh&>(partitioner_.get(), &kglt::Partitioner::remove));

    scene_.signal_light_created().connect(sigc::mem_fun<Light&>(partitioner_.get(), &kglt::Partitioner::add));
    scene_.signal_light_destroyed().connect(sigc::mem_fun<Light&>(partitioner_.get(), &kglt::Partitioner::remove));

    //Set up the default render options
    render_options.wireframe_enabled = false;
    render_options.texture_enabled = true;
    render_options.backface_culling_enabled = true;
    render_options.point_size = 1;

    add_pass(0); //Add a pass for the default scene group by default
}

void Pipeline::remove_all_passes() {
    passes_.clear();
}

void Pipeline::add_pass(SceneGroupID scene_group, TextureID target, CameraID camera, ViewportType viewport) {
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

    pass->viewport_.update_opengl();

    signal_render_pass_started_(index);

    std::set<MeshID> visible_meshes = partitioner_->meshes_visible_from(pass->camera_, pass->scene_group_);

    //TODO: Batched rendering
    renderer_->render(visible_meshes);

    signal_render_pass_finished_(index);
}

}
