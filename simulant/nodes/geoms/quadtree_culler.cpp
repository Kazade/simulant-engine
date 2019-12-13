#include <functional>
#include "quadtree_culler.h"
#include "loose_quadtree.h"

#include "../../vertex_data.h"
#include "../../frustum.h"
#include "../../meshes/mesh.h"
#include "../geom.h"
#include "../../renderers/renderer.h"
#include "../../renderers/batching/renderable_store.h"
#include "../../material.h"
#include "../../stage.h"

namespace smlt {

struct CullerTreeData {
    const VertexData* vertices;
};

struct CullerNodeData {
    std::unordered_map<MaterialID, IndexData::ptr> triangles;
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
        auto material_id = submesh->material();

        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            stash[0] = *data.vertices->position_at<Vec3>(a);
            stash[1] = *data.vertices->position_at<Vec3>(b);
            stash[2] = *data.vertices->position_at<Vec3>(c);

            auto node = pimpl_->quadtree->find_destination_for_triangle(stash);

            auto it = node->data->triangles.find(material_id);
            if(it == node->data->triangles.end()) {
                it = node->data->triangles.emplace(
                    material_id, std::make_shared<IndexData>(index_type_)
                ).first;
            }

            auto& indexes = *it->second;
            indexes.index(a);
            indexes.index(b);
            indexes.index(c);
        });
    }
}

static void node_visitor(QuadtreeCuller* _this, RenderableFactory* factory, CullerQuadtree::Node* node) {
    StagePtr stage = _this->geom()->stage.get();

    for(auto& p: node->data->triangles) {
        auto mat_id = p.first;
        Renderable new_renderable;

        new_renderable.arrangement = smlt::MESH_ARRANGEMENT_TRIANGLES;
        new_renderable.final_transformation = Mat4();
        new_renderable.index_data = p.second.get();
        new_renderable.vertex_data = _this->_vertex_data();
        new_renderable.render_priority = _this->geom()->render_priority();
        new_renderable.index_element_count = new_renderable.index_data->count();
        new_renderable.is_visible = _this->geom()->is_visible();

        // FIXME: Slow material lookup!
        new_renderable.material = stage->assets->material(mat_id).get();

        factory->push_renderable(new_renderable);
    }
}

void QuadtreeCuller::_gather_renderables(const Frustum &frustum, RenderableFactory* factory) {
    pimpl_->quadtree->traverse_visible(frustum, std::bind(&node_visitor, this, factory, std::placeholders::_1));
}

void QuadtreeCuller::_all_renderables(RenderableFactory* factory) {
    pimpl_->quadtree->traverse(std::bind(&node_visitor, this, factory, std::placeholders::_1));
}

AABB QuadtreeCuller::Quadtree_bounds() const {
    return pimpl_->quadtree->bounds();
}

}
