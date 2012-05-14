#ifndef KGLT_GENERIC_RENDERER_H_INCLUDED
#define KGLT_GENERIC_RENDERER_H_INCLUDED

#include <iostream>

#include "object.h"
#include "object_visitor.h"

#include "kglt/utils/matrix_stack.h"

namespace kglt {

class Mesh;
class Camera;
class Scene;

struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class GenericRenderer : public ObjectVisitor {
public:
	typedef std::tr1::shared_ptr<Renderer> ptr;

	GenericRenderer() {}

	GenericRenderer(RenderOptions options):
        options_(options) {

    }

    ~GenericRenderer() {

    }

	void set_options(const RenderOptions& options) { options_ = options; }

    void start_render(Scene* scene);
    void visit(Mesh* mesh);

    //The default camera is set in start_render
    //TODO: Add debug mode which renders camera positions
    void visit(Camera* camera) {}

    //We don't really care about the Scene object, but it is an Object
    //so this method must exist
    void visit(Scene* scene) {}

    //Sprites are just containers
    void visit(Sprite* sprite) {}

    void finish_render() {}

    virtual void pre_visit(Object* obj) {
		modelview_stack_.push();
		
		kmMat4 trans;
		kmMat4Identity(&trans);
		kmMat4Translation(&trans, obj->position().x, obj->position().y, obj->position().z);
		kmMat4Multiply(&modelview_stack_.top(), &modelview_stack_.top(), &trans);				
	}
	
    virtual void post_visit(Object* obj) {
		modelview_stack_.pop();
	}
	    
    void set_perspective_projection(double fov, double aspect, double near=1.0, double far=1000.0f);
    void set_orthographic_projection(double left, double right, double bottom, double top, double near=-1.0, double far=1.0);
	void set_orthographic_projection_from_height(double desired_height_in_units, double ratio);

private:
    RenderOptions options_;
    Scene* scene_;

    MatrixStack modelview_stack_;
    MatrixStack projection_stack_;
    
    kmMat4 projection_matrix_;
};

}

#endif // RENDERER_H_INCLUDED
