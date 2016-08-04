#pragma once

#include <memory>
#include "../../generic/property.h"
#include "../../types.h"
#include "../../interfaces.h"
#include "render_queue.h"

namespace kglt {

class VertexData;
class IndexData;

class Renderable:
    public batcher::BatchMember,
    public virtual BoundableEntity {

public:
    virtual ~Renderable() {}

    virtual const MeshArrangement arrangement() const = 0;

#ifdef KGLT_GL_VERSION_2X
    virtual void _update_vertex_array_object() = 0;
    virtual void _bind_vertex_array_object() = 0;
#endif

    virtual RenderPriority render_priority() const = 0;
    virtual Mat4 final_transformation() const = 0;

    virtual const MaterialID material_id() const = 0;
    virtual const bool is_visible() const = 0;

    virtual MeshID instanced_mesh_id() const = 0;
    virtual SubMeshID instanced_submesh_id() const = 0;

    void update_last_visible_frame_id(uint64_t frame_id) {
        last_visible_frame_id_ = frame_id;
    }

    bool is_visible_in_frame(uint64_t frame_id) const {
        return frame_id == last_visible_frame_id_;
    }

    void set_affected_by_lights(std::vector<LightPtr> lights) {
        lights_affecting_this_frame_ = lights;
    }

    std::vector<LightPtr> lights_affecting_this_frame() const {
        return lights_affecting_this_frame_;
    }

    Property<Renderable, VertexData> vertex_data = { this, &Renderable::get_vertex_data };
    Property<Renderable, IndexData> index_data = { this, &Renderable::get_index_data };

private:
    virtual VertexData* get_vertex_data() const = 0;
    virtual IndexData* get_index_data() const = 0;

    uint64_t last_visible_frame_id_ = 0;
    std::vector<LightPtr> lights_affecting_this_frame_;
};

typedef std::shared_ptr<Renderable> RenderablePtr;

}
