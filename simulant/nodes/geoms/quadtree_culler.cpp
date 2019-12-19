#include <functional>
#include "quadtree_culler.h"
#include "loose_quadtree.h"

#include "../../vertex_data.h"
#include "../../frustum.h"
#include "../../meshes/mesh.h"
#include "../geom.h"
#include "../../renderers/renderer.h"
#include "../../material.h"
#include "../../stage.h"

namespace smlt {

struct CullerTreeData {
    const VertexData* vertices;
};

struct TriangleData {
    TriangleData(Material* material, IndexData::ptr indexes):
        material(material), indexes(indexes) {}

    Material* material = nullptr;
    IndexData::ptr indexes;
};

struct CullerNodeData {
    std::unordered_map<MaterialID, TriangleData> triangles;
};


typedef Quadtree<CullerTreeData, CullerNodeData> CullerQuadtree;


struct _QuadtreeCullerImpl {
    std::shared_ptr<CullerQuadtree> quadtree;
};

QuadtreeCuller::QuadtreeCuller(Geom *geom, const MeshPtr mesh, uint8_t max_depth):
    GeomCuller(geom, mesh),
    pimpl_(new _QuadtreeCullerImpl()),
    max_depth_(max_depth) {

    /* Find the size of index we need to store all indices */
    IndexType type = INDEX_TYPE_8_BIT;
    for(auto submesh: mesh_->each_submesh()) {
        if(submesh->index_data->index_type() > type) {
            type = submesh->index_data->index_type();
        }
    }

    index_type_ = type;
}

const VertexData *QuadtreeCuller::_vertex_data() const {
    assert(mesh_);
    return mesh_->vertex_data.get();
}

void QuadtreeCuller::_compile() {
    CullerTreeData data;
    data.vertices = _vertex_data();

    AABB bounds(*data.vertices);
    pimpl_->quadtree.reset(new CullerQuadtree(bounds, max_depth_, &data));

    Vec3 stash[3];

    for(auto submesh: mesh_->each_submesh()) {
        auto material = submesh->material();

        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            stash[0] = *data.vertices->position_at<Vec3>(a);
            stash[1] = *data.vertices->position_at<Vec3>(b);
            stash[2] = *data.vertices->position_at<Vec3>(c);

            auto node = pimpl_->quadtree->find_destination_for_triangle(stash);

            auto it = node->data->triangles.find(material->id());
            if(it == node->data->triangles.end()) {
                it = node->data->triangles.emplace(
                    material->id(),
                    TriangleData(material.get(), std::make_shared<IndexData>(index_type_)
                )).first;
            }

            auto& indexes = *it->second.indexes;
            indexes.index(a);
            indexes.index(b);
            indexes.index(c);
        });
    }
}

void QuadtreeCuller::_gather_renderables(const Frustum &frustum, batcher::RenderQueue* render_queue) {
    auto cb = [this, render_queue](CullerQuadtree::Node* node) {
        for(auto& p: node->data->triangles) {
            Renderable new_renderable;

            new_renderable.arrangement = smlt::MESH_ARRANGEMENT_TRIANGLES;
            new_renderable.final_transformation = Mat4();
            new_renderable.index_data = p.second.indexes.get();
            new_renderable.vertex_data = this->_vertex_data();
            new_renderable.render_priority = this->geom()->render_priority();
            new_renderable.index_element_count = new_renderable.index_data->count();
            new_renderable.is_visible = this->geom()->is_visible();
            new_renderable.material = p.second.material;

            render_queue->insert_renderable(std::move(new_renderable));
        }
    };

    pimpl_->quadtree->traverse_visible(frustum, cb);
}

void QuadtreeCuller::_all_renderables(batcher::RenderQueue* render_queue) {
    auto cb = [this, render_queue](CullerQuadtree::Node* node) {
        for(auto& p: node->data->triangles) {
            Renderable new_renderable;

            new_renderable.arrangement = smlt::MESH_ARRANGEMENT_TRIANGLES;
            new_renderable.final_transformation = Mat4();
            new_renderable.index_data = p.second.indexes.get();
            new_renderable.vertex_data = this->_vertex_data();
            new_renderable.render_priority = this->geom()->render_priority();
            new_renderable.index_element_count = new_renderable.index_data->count();
            new_renderable.is_visible = this->geom()->is_visible();
            new_renderable.material = p.second.material;

            render_queue->insert_renderable(std::move(new_renderable));
        }
    };

    pimpl_->quadtree->traverse(cb);
}

AABB QuadtreeCuller::Quadtree_bounds() const {
    return pimpl_->quadtree->bounds();
}

}
