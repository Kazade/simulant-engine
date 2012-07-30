#ifndef KGLT_RENDERER_H
#define KGLT_RENDERER_H

#include "object.h"
#include "object_visitor.h"

#include "kglt/utils/matrix_stack.h"

namespace kglt {
	
struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class Renderer : public ObjectVisitor {
public:
	typedef std::tr1::shared_ptr<Renderer> ptr;

	Renderer(Scene& scene);
	Renderer(Scene& scene, const RenderOptions& options);
	virtual ~Renderer() {}
	
	void set_options(const RenderOptions& options) { options_ = options; }
	
	virtual void start_render();
	virtual void finish_render() { on_finish_render(); }

    virtual bool pre_visit(Object* obj) {
        obj->pre_visit(*this);

		modelview_stack_.push();
		
		kmMat4 trans;
		kmMat4Identity(&trans);
		kmMat4Translation(&trans, obj->position().x, obj->position().y, obj->position().z);
		kmMat4Multiply(&modelview_stack_.top(), &modelview_stack_.top(), &trans);
		
		return true;
	}
	
    virtual void post_visit(Object* obj) {
		modelview_stack_.pop();

        obj->post_visit(*this);
	}
		
	RenderOptions& options() { return options_; }
	Scene& scene() { return scene_; }
	
	MatrixStack modelview_stack() { return modelview_stack_; }
	
    void set_projection_matrix(kmMat4 projection_matrix) { projection_matrix_ = projection_matrix; }
    const kmMat4& projection_matrix() const { return projection_matrix_; }

private:
    RenderOptions options_;
    Scene& scene_;

    MatrixStack modelview_stack_;
    kmMat4 projection_matrix_;
    
    virtual void on_start_render() {}
    virtual void on_finish_render() {}
};

}
#endif
