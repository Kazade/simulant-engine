#include <functional>
#include "quadtree_culler.h"
#include "culler_triangle_buffers.h"
#include "loose_quadtree.h"

#include "../../vertex_data.h"
#include "../../frustum.h"
#include "../../meshes/mesh.h"
#include "../geom.h"
#include "../../renderers/renderer.h"
#include "../../assets/material.h"
#include "../../stage.h"

namespace smlt {

struct CullerTreeData {
    std::unique_ptr<VertexData> vertices;
    std::unique_ptr<CullerMaterialBuffers> material_buffers;
};

struct CullerNodeData {
    CullerNodeTriangles triangles;
};


typedef Quadtree<CullerTreeData, CullerNodeData> CullerQuadtree;


struct _QuadtreeCullerImpl {
    std::shared_ptr<CullerQuadtree> quadtree;
};

QuadtreeCuller::QuadtreeCuller(Geom *geom, const MeshPtr mesh, uint8_t max_depth):
    GeomCuller(geom, mesh),
    pimpl_(new _QuadtreeCullerImpl()),
    max_depth_(max_depth) {

    /* Find the size of index we need to store all indices. The checks must
     * go from largest to smallest, otherwise the 32-bit branch is
     * unreachable. */
    IndexType type = INDEX_TYPE_8_BIT;
    if(mesh->vertex_data->count() > std::numeric_limits<uint16_t>::max()) {
        type = INDEX_TYPE_32_BIT;
    } else if(mesh->vertex_data->count() > std::numeric_limits<uint8_t>::max()) {
        type = INDEX_TYPE_16_BIT;
    }

    index_type_ = type;
}

void QuadtreeCuller::_compile(const Vec3& pos, const Quaternion& rot, const Vec3& scale) {
    auto data = std::make_shared<CullerTreeData>();

    data->vertices.reset(new VertexData(mesh_->vertex_data->vertex_specification()));
    mesh_->vertex_data->clone_into(*data->vertices);

    Mat4 transform = Mat4::as_transform(pos, rot, scale);
    data->vertices->transform_by(transform);

    data->material_buffers.reset(new CullerMaterialBuffers(index_type_));

    AABB bounds(*data->vertices);
    pimpl_->quadtree.reset(new CullerQuadtree(bounds, max_depth_, data));

    Vec3 stash[3];

    for(auto submesh: mesh_->each_submesh()) {
        auto material = submesh->material();

        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            stash[0] = *data->vertices->position_at<Vec3>(a);
            stash[1] = *data->vertices->position_at<Vec3>(b);
            stash[2] = *data->vertices->position_at<Vec3>(c);

            auto node = pimpl_->quadtree->find_destination_for_triangle(stash);

            auto& entry = node->data->triangles.entry_for(material.get());
            entry.pending.push_back(a);
            entry.pending.push_back(b);
            entry.pending.push_back(c);
        });
    }

    /* Walk the tree in traversal order so each material's index buffer is
     * ordered by node, allowing adjacent visible nodes to be batched. */
    pimpl_->quadtree->traverse([&](CullerQuadtree::Node* node) {
        for(auto& entry: node->data->triangles.entries) {
            data->material_buffers->add(entry);
        }
    });

    data->material_buffers->done();
}

void QuadtreeCuller::_gather_renderables(const Frustum &frustum, batcher::RenderQueue* render_queue) {
    auto* data = pimpl_->quadtree->data();

    CullerRangeAccumulator accumulator(
        [this, data, render_queue](Material* material, IndexData* indexes,
                                   uint32_t first_index, uint32_t count) {
            Renderable new_renderable;

            new_renderable.arrangement = smlt::MESH_ARRANGEMENT_TRIANGLES;
            static const Mat4 s_identity;
            new_renderable.final_transformation = &s_identity;
            new_renderable.index_data = indexes;
            new_renderable.vertex_data = data->vertices.get();
            new_renderable.render_priority = this->geom()->render_priority();
            new_renderable.first_index = first_index;
            new_renderable.index_element_count = count;
            new_renderable.is_visible = this->geom()->is_visible();
            new_renderable.material = material;
            new_renderable.key = culler_range_key(indexes, first_index);
            if(this->geom()->shadow_receive() == SHADOW_RECEIVE_ALWAYS) {
                new_renderable.flags |= RENDERABLE_FLAG_RECEIVES_SHADOWS;
            }

            render_queue->insert_renderable(std::move(new_renderable));
        });

    auto cb = [&accumulator](CullerQuadtree::Node* node) {
        for(auto& entry: node->data->triangles.entries) {
            accumulator.add(entry.material, entry.indexes.get(),
                            entry.first_index, entry.count);
        }
    };

    pimpl_->quadtree->traverse_visible(frustum, cb);
    accumulator.finish();
}

void QuadtreeCuller::_all_renderables(batcher::RenderQueue* render_queue) {
    auto* data = pimpl_->quadtree->data();

    CullerRangeAccumulator accumulator(
        [this, data, render_queue](Material* material, IndexData* indexes,
                                   uint32_t first_index, uint32_t count) {
            Renderable new_renderable;

            new_renderable.arrangement = smlt::MESH_ARRANGEMENT_TRIANGLES;
            static const Mat4 s_identity;
            new_renderable.final_transformation = &s_identity;
            new_renderable.index_data = indexes;
            new_renderable.vertex_data = data->vertices.get();
            new_renderable.render_priority = this->geom()->render_priority();
            new_renderable.first_index = first_index;
            new_renderable.index_element_count = count;
            new_renderable.is_visible = this->geom()->is_visible();
            new_renderable.material = material;
            new_renderable.key = culler_range_key(indexes, first_index);
            if(this->geom()->shadow_receive() == SHADOW_RECEIVE_ALWAYS) {
                new_renderable.flags |= RENDERABLE_FLAG_RECEIVES_SHADOWS;
            }

            render_queue->insert_renderable(std::move(new_renderable));
        });

    auto cb = [&accumulator](CullerQuadtree::Node* node) {
        for(auto& entry: node->data->triangles.entries) {
            accumulator.add(entry.material, entry.indexes.get(),
                            entry.first_index, entry.count);
        }
    };

    pimpl_->quadtree->traverse(cb);
    accumulator.finish();
}

AABB QuadtreeCuller::Quadtree_bounds() const {
    return pimpl_->quadtree->bounds();
}

}
