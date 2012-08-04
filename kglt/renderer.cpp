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

bool BaseRenderer::pre_visit(Object& obj) {
    modelview().push();

    kmMat4 trans;
    kmMat4Identity(&trans);
    kmMat4Translation(&trans, obj.absolute_position().x, obj.absolute_position().y, obj.absolute_position().z);
    kmMat4Multiply(&modelview().top(), &modelview().top(), &trans);

    return true;
}

void BaseRenderer::post_visit(Object& object) {
    modelview().pop();
}

void Renderer::visit(Object object) {

}


}
