#include <functional>
#include "octree_culler.h"
#include "loose_octree.h"

#include "../../vertex_data.h"
#include "../../frustum.h"
#include "../../meshes/mesh.h"
#include "../geom.h"
#include "../../renderers/renderer.h"
#include "geom_culler_renderable.h"
#include "../../renderers/batching/renderable_store.h"

namespace smlt {

struct CullerTreeData {
    const VertexData* vertices;
};

struct CullerNodeData {
    std::unordered_map<MaterialID, std::vector<uint32_t> > triangles;
};


typedef Octree<CullerTreeData, CullerNodeData> CullerOctree;


struct _OctreeCullerImpl {
    std::unordered_map<MaterialID, std::shared_ptr<GeomCullerRenderable>> renderable_map;
    std::shared_ptr<CullerOctree> octree;
};

OctreeCuller::OctreeCuller(Geom *geom, const MeshPtr mesh, uint8_t max_depth):
    GeomCuller(geom, mesh),
    pimpl_(new _OctreeCullerImpl()),
    max_depth_(max_depth) {

    /* Find the size of index we need to store all indices */
    IndexType type = INDEX_TYPE_8_BIT;
    mesh->each([&](const std::string&, SubMesh* submesh) {
        if(submesh->index_data->index_type() > type) {
            type = submesh->index_data->index_type();
        }
    });

    index_type_ = type;
}

const VertexData *OctreeCuller::_vertex_data() const {
    assert(mesh_);
    return mesh_->vertex_data.get();
}

void OctreeCuller::_compile() {
    CullerTreeData data;
    data.vertices = _vertex_data();

    AABB bounds(*data.vertices);
    pimpl_->octree.reset(new CullerOctree(bounds, max_depth_, &data));

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

void OctreeCuller::_all_renderables(RenderableFactory* factory) {
    auto& renderable_map = pimpl_->renderable_map;
    for(auto& p: renderable_map) {
        auto& gr = p.second;

        Renderable new_renderable;

        new_renderable.arrangement = gr->arrangement();
        new_renderable.final_transformation = gr->final_transformation();
        new_renderable.index_data = gr->index_data();
        new_renderable.vertex_data = _vertex_data();
        new_renderable.render_priority = gr->render_priority();
        new_renderable.index_element_count = new_renderable.index_data->count();
        new_renderable.is_visible = gr->is_visible();
        new_renderable.material_id = gr->material_id();
        new_renderable.centre = gr->transformed_aabb().centre();

        factory->push_renderable(new_renderable);
    }
}

void OctreeCuller::_gather_renderables(const Frustum &frustum, RenderableFactory* factory) {
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
            }

            /* Transfer the indices to the renderable */
            renderable->_indices().index(&p.second[0], p.second.size());
            renderable->_indices().done();
        }
    };

    pimpl_->octree->traverse_visible(frustum, visitor);

    for(auto& gr: seen) {
        Renderable new_renderable;

        new_renderable.arrangement = gr->arrangement();
        new_renderable.final_transformation = gr->final_transformation();
        new_renderable.index_data = gr->index_data();
        new_renderable.vertex_data = _vertex_data();
        new_renderable.render_priority = gr->render_priority();
        new_renderable.index_element_count = new_renderable.index_data->count();
        new_renderable.is_visible = gr->is_visible();
        new_renderable.material_id = gr->material_id();

        factory->push_renderable(new_renderable);
    }
}

AABB OctreeCuller::octree_bounds() const {
    return pimpl_->octree->bounds();
}

}
