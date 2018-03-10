#include <functional>
#include "octree_culler.h"
#include "loose_octree.h"

#include "../../vertex_data.h"
#include "../../frustum.h"
#include "../../meshes/mesh.h"
#include "../geom.h"
#include "../../renderers/renderer.h"
#include "../../hardware_buffer.h"
#include "geom_culler_renderable.h"

namespace smlt {

struct CullerTreeData {
    VertexData* vertices;
};

struct CullerNodeData {
    std::unordered_map<MaterialID, std::vector<uint32_t> > triangles;
};


typedef Octree<CullerTreeData, CullerNodeData> CullerOctree;


struct _OctreeCullerImpl {
    std::unordered_map<MaterialID, std::shared_ptr<GeomCullerRenderable>> renderable_map;
    std::shared_ptr<CullerOctree> octree;
};

OctreeCuller::OctreeCuller(Geom *geom, const MeshPtr mesh):
    GeomCuller(geom, mesh),
    pimpl_(new _OctreeCullerImpl()),
    vertices_(mesh->vertex_data->specification()) {

    /* We have to clone the vertex data as the mesh will be destroyed */
    mesh->vertex_data->clone_into(vertices_);

    /* Find the size of index we need to store all indices */
    IndexType type = INDEX_TYPE_8_BIT;
    mesh->each([&](const std::string&, SubMesh* submesh) {
        if(submesh->index_data->index_type() > type) {
            type = submesh->index_data->index_type();
        }
    });
}

const VertexData *OctreeCuller::_vertex_data() const {
    return &vertices_;
}

HardwareBuffer *OctreeCuller::_vertex_attribute_buffer() const {
    return vertex_attribute_buffer_.get();
}

void OctreeCuller::_compile() {
    CullerTreeData data;
    data.vertices = &vertices_;

    AABB bounds(*data.vertices);
    pimpl_->octree.reset(new CullerOctree(bounds, 4, &data));

    Vec3 stash[3];

    auto& renderable_map = pimpl_->renderable_map;

    mesh_->each([&](const std::string&, SubMesh* submesh) {
        auto material_id = submesh->material_id();

        /* Generate a renderable for each material. These are added
         * to the RenderQueue and updated before each render queu
         * iteration by _gather_renderables
         *
         * This must be done here, because when the geom_created() signal is
         * fired by the stage, then the render queue is updated which will call
         * _all_renderables.
        */

        auto it = renderable_map.find(material_id);
        if(it == renderable_map.end()) {
            // Not in the map yet? Create a new renderable
            auto r = std::make_shared<GeomCullerRenderable>(
                this,
                material_id,
                index_type_
            );

            renderable_map.insert(std::make_pair(material_id, r));
        }

        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            stash[0] = data.vertices->position_at<Vec3>(a);
            stash[1] = data.vertices->position_at<Vec3>(b);
            stash[2] = data.vertices->position_at<Vec3>(c);

            auto node = pimpl_->octree->find_destination_for_triangle(stash);

            auto& indexes = node->data->triangles[material_id];
            indexes.push_back(a);
            indexes.push_back(b);
            indexes.push_back(c);
        });
    });
}

void OctreeCuller::_all_renderables(RenderableList& out) {
    auto& renderable_map = pimpl_->renderable_map;
    for(auto& p: renderable_map) {
        out.push_back(p.second);
    }
}

void OctreeCuller::_gather_renderables(const Frustum &frustum, std::vector<std::shared_ptr<Renderable> > &out) {    
    auto& renderable_map = pimpl_->renderable_map;

    /* Reset all the index data before we start gathering */
    for(auto& p: renderable_map) {
        p.second->_indices().reset();
    }

    std::unordered_set<GeomCullerRenderable*> seen;

    auto visitor = [&](CullerOctree::Node* node) {
        for(auto& p: node->data->triangles) {
            auto mat_id = p.first;
            auto it = renderable_map.find(mat_id);
            GeomCullerRenderable* renderable = it->second.get();

            if(!seen.count(renderable)) {
                seen.insert(renderable);
                out.push_back(it->second);
            }

            /* Transfer the indices to the renderable */
            renderable->_indices().index(&p.second[0], p.second.size());
            renderable->_indices().done();
        }
    };

    pimpl_->octree->traverse_visible(frustum, visitor);
}

void OctreeCuller::_prepare_buffers(Renderer* renderer) {
    if(!vertex_attribute_buffer_ && is_compiled()) {
        vertex_attribute_buffer_ = renderer->hardware_buffers->allocate(
            vertices_.data_size(),
            HARDWARE_BUFFER_VERTEX_ATTRIBUTES,
            SHADOW_BUFFER_DISABLED,
            HARDWARE_BUFFER_MODIFY_ONCE_USED_FOR_RENDERING
        );

        vertex_attribute_buffer_->upload(vertices_);
    }
}

AABB OctreeCuller::octree_bounds() const {
    return pimpl_->octree->bounds();
}

}
