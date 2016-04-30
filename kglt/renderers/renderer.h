#ifndef RENDERER_H
#define RENDERER_H

#include <set>
#include <vector>
#include <memory>

#include "../generic/protected_ptr.h"
#include "../types.h"
#include "../utils/geometry_buffer.h"
#include "../generic/auto_weakptr.h"
#include "../window_base.h"

namespace kglt {

class SubActor;

class Renderable : public virtual BoundableEntity {
public:
    virtual ~Renderable() {}

    virtual const VertexData& vertex_data() const = 0;
    virtual const IndexData& index_data() const = 0;
    virtual const MeshArrangement arrangement() const = 0;

    virtual void _update_vertex_array_object() = 0;
    virtual void _bind_vertex_array_object() = 0;

    virtual RenderPriority render_priority() const = 0;
    virtual Mat4 final_transformation() const = 0;

    virtual const MaterialID material_id() const = 0;
    virtual const bool is_visible() const = 0;

    virtual MeshID instanced_mesh_id() const = 0;
    virtual SubMeshID instanced_submesh_id() const = 0;
};

typedef std::shared_ptr<Renderable> RenderablePtr;


class Renderer {
public:
    typedef std::shared_ptr<Renderer> ptr;

    Renderer(WindowBase* window):
        window_(window) {}

    void set_current_stage(StageID stage) {
        current_stage_ = stage;
    }

    virtual void render(Renderable& buffer, CameraID camera, GPUProgramInstance* program) = 0;

    Property<Renderer, WindowBase> window = { this, &Renderer::window_ };
protected:
    StagePtr current_stage();

private:    
    WindowBase* window_ = nullptr;
    StageID current_stage_;

};

}

#endif // RENDERER_H
