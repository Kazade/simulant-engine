#ifndef SPRITE_H
#define SPRITE_H

#include "object.h"
#include "loadable.h"
#include "object_visitor.h"
#include "kazbase/logging/logging.h"

namespace kglt {

class Scene;

class Sprite : public Loadable, public Object {
public:
    typedef std::tr1::shared_ptr<Sprite> ptr;

    Sprite();

    uint32_t frame_count() const;
    void set_animation_frames(uint32_t first_frame, uint32_t last_frame);
    void set_animation_fps(float fps);

    void set_frame_count(uint32_t frame_count);
    void set_frame_size(uint32_t width, uint32_t height);
    void load_frame(uint32_t frame_id, std::vector<uint8_t> data, uint32_t bpp);
    void set_render_dimensions(float width, float height);

    virtual void on_parent_set(Object* old_parent);

    void accept(ObjectVisitor& visitor) {
		visitor.pre_visit(this);
		
        for(Object* child: children_) {
            child->accept(visitor);
        }

        visitor.visit(this);
        visitor.post_visit(this);
    }
    
    MeshID mesh_id() const { return mesh_id_; }
private:
    bool initialized_;

    uint32_t frame_width_;
    uint32_t frame_height_;

    float animation_fps_;
    float render_width_;
    float render_height_;

    MeshID mesh_id_;
    std::vector<TextureID> frames_;
};

}

#endif
