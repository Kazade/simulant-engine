#include "sprite.h"
#include "scene.h"

#include "procedural/mesh.h"

namespace kglt {

Sprite::Sprite():
	initialized_(false),
	frame_width_(0),
	frame_height_(0),
	animation_fps_(1.0),
	first_animation_frame_(0),
	last_animation_frame_(0),
	frame_interp_(0.0),
	render_width_(1.0),
	render_height_(1.0) {
	
}

Sprite::~Sprite() {
	
}

void Sprite::set_animation_frames(uint32_t first_frame, uint32_t last_frame) {
	assert(first_frame < frame_count());
	assert(last_frame < frame_count());
	
	first_animation_frame_ = first_frame;
	last_animation_frame_ = last_frame;
	current_frame_ = first_animation_frame_;
	
	_update_frame(current_frame_);
}

void Sprite::_set_texture_id(TextureID tex_id) { 
	sprite_texture_ = tex_id; 
	
	scene().mesh(mesh_id_).apply_texture(tex_id, kglt::TextureLevel::PRIMARY);
}
	
void Sprite::do_update(double dt) {
	if(first_animation_frame_ == last_animation_frame_) {
		_update_frame(first_animation_frame_);
		return;
	}
	
	frame_interp_ += dt;
	if(frame_interp_ >= (1.0 / animation_fps_)) {
		current_frame_++;
		if(current_frame_ > frame_count() || current_frame_ > last_animation_frame_) {
			current_frame_ = first_animation_frame_;
		}
		_update_frame(current_frame_);
		frame_interp_ = 0.0;
	}	
}

void Sprite::_update_frame(uint32_t current_frame) {
    double frame_width = 1.0 / double(frame_count_);
	
	Mesh& mesh = scene().mesh(mesh_id_);
	
    mesh.triangle(0).uv(0).x = frame_width * current_frame;
    mesh.triangle(0).uv(1).x = frame_width * (current_frame + 1);
    mesh.triangle(0).uv(2).x = frame_width * (current_frame + 1);
	
    mesh.triangle(1).uv(0).x = frame_width * current_frame;
    mesh.triangle(1).uv(1).x = frame_width * (current_frame + 1);
    mesh.triangle(1).uv(2).x = frame_width * current_frame;
	
	mesh.invalidate();
}

void Sprite::set_render_dimensions(float width, float height) {
    render_width_ = width;
    render_height_ = height;
    
    Scene* scene = root_as<Scene>();
    assert(scene);
    
    //Rebuild the mesh
    Mesh& mesh = scene->mesh(mesh_id_);
    kglt::procedural::mesh::rectangle(mesh, render_width_, render_height_);
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
