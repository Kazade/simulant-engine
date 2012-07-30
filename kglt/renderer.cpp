#include "renderer.h"
#include "scene.h"
#include "window_base.h"

namespace kglt {
	
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
}



}
