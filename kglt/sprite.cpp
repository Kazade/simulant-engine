#include "sprite.h"
#include "scene.h"

#include "procedural/mesh.h"

namespace kglt {

Sprite::Sprite():
	frame_width_(0),
	frame_height_(0),
	animation_fps_(0),
	render_width_(1.0),
	render_height_(1.0) {

}

void Sprite::set_frame_count(uint32_t frames) {
	assert(frames > frames_.size()); //FIXME: We should properly destroy textures etc.
	frames_.resize(frames);
}

void Sprite::set_render_dimensions(float width, float height) {
    render_width_ = width;
    render_height_ = height;
}

void Sprite::on_parent_set(Object* old_parent) {
    if(has_parent() && !initialized_) {
        L_DEBUG("Initializing sprite");

        //Get the scene
        Scene* scene = root_as<Scene>();
        assert(scene);

        //Create a mesh
        mesh_id_ = scene->new_mesh();

        //Make the mesh a rectangle, set the parent as this sprite
        Mesh& mesh = scene->mesh(mesh_id_);

        kglt::procedural::mesh::rectangle(mesh, render_width_, render_height_);

        mesh.set_parent(this);

        //Mark the sprite as being initialized
        initialized_ = true;
    } else if(!initialized_ && !has_parent()) {
        L_WARN("Sprite has not been attached to a parent, and has not been initialized");
    }
}

}
