#ifndef KGLT_RENDERER_H
#define KGLT_RENDERER_H

#include "object.h"
#include "kglt/utils/matrix_stack.h"

#include "generic/visitor.h"
#include "generic/tree.h"

namespace kglt {
	
struct RenderOptions {
    bool wireframe_enabled;
    bool texture_enabled;
    bool backface_culling_enabled;
    uint8_t point_size;
};

class BaseRenderer : public generic::Visitor<Object> {
public:
    BaseRenderer(const RenderOptions& options=RenderOptions()):
        options_(options) {

    }

    virtual ~BaseRenderer() {}

    void render(Scene& scene);

    void set_options(RenderOptions& options) {
        options_ = options;
    }

protected:
    RenderOptions& options() { return options_; }

    MatrixStack& modelview() { return modelview_stack_; }
    MatrixStack& projection() { return projection_stack_; }

    virtual void on_start_render(Scene& scene) {}
    virtual void on_finish_render(Scene& scene) {}
    virtual bool pre_visit(Object& object) { return true; }
    virtual void post_visit(Object& object) {}

private:
    RenderOptions options_;

    MatrixStack modelview_stack_;
    MatrixStack projection_stack_;
};


class Renderer : public BaseRenderer {
public:
    typedef std::tr1::shared_ptr<Renderer> ptr;

    Renderer(const RenderOptions& options = RenderOptions()):
        BaseRenderer(options) {}

    /*
      Non-virtual catch-all for anything
      that isn't one of the classes listed in the
      template list
    */
    void visit(Object object) {}
};

/*
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
};*/

}
#endif
