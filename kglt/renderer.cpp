#include "renderer.h"
#include "scene.h"
#include "window_base.h"

namespace kglt {
	
void BaseRenderer::render(Scene& scene) {
    on_start_render(scene);

    //FIXME: This is ugly and inconsistent
    scene.active_camera().apply(&modelview().top());
    kmMat4Assign(&projection().top(), &scene.active_camera().projection_matrix());

    for(Scene::iterator it = scene.begin(); it != scene.end(); ++it) {
        Object& object = static_cast<Object&>(*it);
        if(pre_visit(object)) {
            (*this)(object);
            post_visit(object);
        }
    }

    on_finish_render(scene);
}

/*
Renderer::Renderer(Scene& scene):
	scene_(scene) {


}

Renderer::Renderer(Scene& scene, const RenderOptions& options):
    options_(options),
    scene_(scene) {	
}

void Renderer::start_render() {
    //Apply the camera transform
    kmMat4* modelview = &modelview_stack_.top();
    scene().active_camera().apply(modelview);

    //Initialize the projection matrix
    set_projection_matrix(scene().active_camera().projection_matrix());

    on_start_render();
}*/



}
