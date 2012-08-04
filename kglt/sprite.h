#ifndef SPRITE_H
#define SPRITE_H

#include "object.h"
#include "loadable.h"
#include "generic/visitor.h"
#include "kazbase/logging/logging.h"

namespace kglt {

class Scene;
class Texture;

class Sprite :
    public Loadable,
    public Object,
    public generic::VisitableBase<Sprite> {

public:
    typedef std::tr1::shared_ptr<Sprite> ptr;

    Sprite();
	virtual ~Sprite();
	
    uint32_t frame_count() const { return frame_count_; }
    void set_animation_frames(uint32_t first_frame, uint32_t last_frame);
    void set_animation_fps(float fps) { animation_fps_ = fps; }
    void set_render_dimensions(float width, float height);
	
	//===============================================================
	//Don't call these methods unless you are writing a sprite loader!	
	void _set_texture_id(TextureID tex_id);
	
	void _set_frame_size(uint32_t width, uint32_t height) { 
		frame_width_ = width;
		frame_height_ = height;
	}
	
	void _set_frame_count(uint32_t frame_count) {
		frame_count_ = frame_count;
	}
	//===============================================================
	
    virtual void on_parent_set(Object* old_parent);
    
    MeshID mesh_id() const { return mesh_id_; }
    
    void _update_frame(uint32_t frame);
    
    Texture& texture();

private:
    bool initialized_;

    uint32_t frame_width_;
    uint32_t frame_height_;

    float animation_fps_;
    uint32_t first_animation_frame_;
    uint32_t last_animation_frame_;
    uint32_t current_frame_;
    float frame_interp_;
    
    float render_width_;
    float render_height_;

    MeshID mesh_id_;
    TextureID sprite_texture_;
    uint32_t frame_count_;
    
    void do_update(double dt);
};

}

#endif
