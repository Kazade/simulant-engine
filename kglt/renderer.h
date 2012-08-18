#ifndef KGLT_RENDERER_H
#define KGLT_RENDERER_H

#include "kglt/utils/matrix_stack.h"

#include "generic/visitor.h"
#include "generic/tree.h"

#include "object.h"
#include "text.h"
#include "mesh.h"
#include "background.h"
#include "overlay.h"

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

         Visits(*this, Loki::Seq<Object, Mesh, Text, Background, Overlay>::Type());
    }

    virtual ~BaseRenderer() {}

    void render(Scene& scene);

    void set_options(RenderOptions& options) {
        options_ = options;
    }

    virtual void visit(Object& object) {}
    virtual void visit(Mesh& object) = 0;
    virtual void visit(Text& object) = 0;
    virtual void visit(Background& object) = 0;
    virtual void visit(Overlay& overlay) {
        kmMat4Assign(&projection().top(), &overlay.projection_matrix());
    }

    virtual void _initialize(Scene& scene) {}

protected:
    RenderOptions& options() { return options_; }

    MatrixStack& modelview() { return modelview_stack_; }
    MatrixStack& projection() { return projection_stack_; }

    virtual void on_start_render(Scene& scene) {}
    virtual void on_finish_render(Scene& scene) {}
    virtual bool pre_visit(Object& obj);
    virtual void post_visit(Object& object);

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
    void visit(Object& object);
};

}
#endif
