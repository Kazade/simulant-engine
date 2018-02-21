#include <functional>
#include "octree_culler.h"
#include "../../vertex_data.h"
#include "../../frustum.h"
#include "../../meshes/mesh.h"
#include "../geom.h"
#include "../../renderers/renderer.h"
#include "../../hardware_buffer.h"
#include "geom_culler_renderable.h"

namespace smlt {

static constexpr int64_t ipow(int64_t base, int exp, int64_t result = 1) {
  return exp < 1 ? result : ipow(base*base, exp/2, (exp % 2) ? result*base : result);
}

static std::pair<uint8_t, float> level_for_width(float root_width, float obj_width, uint8_t max_level) {
    /*
     * Given the diameter of the root node, and the diameter of the object
     * this returns the level the object will fit, and the node width at that level
     *
     * I *know* there's a non-iterative way to calculate this, so if you know, let me know!
     */

    assert(obj_width <= root_width);

    if(obj_width > root_width) {
        // Should never happen, but this is the safest thing to do if it does
        return std::make_pair(0, root_width);
    }

    auto node_width = root_width;
    uint8_t depth = 0;

    while(node_width >= obj_width) {
        ++depth;
        node_width *= 0.5f;

        if(depth == max_level) {
            break;
        }
    }

    assert(depth >= 1);

    // Off-by-one
    return std::make_pair(depth - 1, node_width * 2);
}

/*
 * TreeData is a structure which contains data needed across the whole tree,
 * this will usually contain the vertex data for the mesh being inserted.
 *
 * NodeData is a structure for storing data on the individual nodes, this is usually
 * index data for the mesh.
 */

template<typename TreeData, typename NodeData>
class Octree {
public:
    struct Node {
        uint8_t level = 0;
        uint8_t grid[3] = {0, 0, 0};
        NodeData* data = nullptr;
    };

    typedef TreeData tree_data_type;
    typedef NodeData node_data_type;

    Octree(const AABB& bounds, uint8_t max_level_count=4, TreeData* tree_data=nullptr):
        tree_data_(tree_data),
        bounds_(bounds),
        centre_(bounds_.centre()) {

        /* Make sure the bounds are square */
        float maxd = std::max(bounds.width(), std::max(bounds.height(), bounds.depth()));
        auto halfd = maxd / 2.0;

        auto half = Vec3(halfd, halfd, halfd);
        bounds_.set_min(centre_ - half);
        bounds_.set_max(centre_ + half);

        /* Grow the tree to whatever size we passed in */
        while(levels_ < max_level_count) {
            grow();
        }
    }

    ~Octree() {
        /* Make sure we delete the node data */
        for(auto& node: nodes_) {
            delete node.data;
            node.data = nullptr;
        }
    }

    bool is_leaf(Node& node) const {
        return node.level == levels_;
    }

    Octree::Node* find_destination_for_sphere(const Vec3& centre, float radius) {
        auto root_width = bounds().width();
        auto diameter = radius * 2;
        auto level_and_node_width = level_for_width(root_width, diameter, levels_);

        /* Calculate the cell index to insert the sphere */
        auto half_width = root_width * 0.5;
        auto x = (int) ((centre.x + half_width) / level_and_node_width.second);
        auto y = (int) ((centre.y + half_width) / level_and_node_width.second);
        auto z = (int) ((centre.z + half_width) / level_and_node_width.second);

        auto idx = calc_index(level_and_node_width.first, x, y, z);
        assert(idx < nodes_.size());

        return &nodes_[idx];
    }

    Octree::Node* find_destination_for_triangle(const Vec3* vertices) {
        /*
         * Return the node that this triangle should be
         * inserted into (the actual insertion won't happen as that's implementation specific
         * depending on the NodeData).
         */

        AABB bounds(vertices, 3);
        auto centre = bounds.centre();
        auto radius = bounds.max_dimension() / 2.0f;
        return find_destination_for_sphere(centre, radius);
    }

    void traverse(std::function<void (Octree::Node*)> cb) {
        if(nodes_.empty()) {
            return;
        }

        std::function<void (Octree::Node&)> visitor = [&](Octree::Node& node) {
            cb(&node);

            if(!is_leaf(node)) {
                std::vector<uint32_t> indexes;
                child_indexes(node, indexes);

                for(auto child: indexes) {
                    assert(child < nodes_.size());
                    visitor(nodes_[child]);
                }
            }
        };

        visitor(nodes_[0]);
    }

    void traverse_visible(const Frustum& frustum, std::function<void (Octree::Node*)> cb) {
        if(nodes_.empty()) {
            return;
        }

        std::function<void (Octree::Node&)> visitor = [&](Octree::Node& node) {
            auto bounds = calc_loose_bounds(node);
            if(frustum.intersects_aabb(bounds)) {
                cb(&node);

                if(!is_leaf(node)) {
                    std::vector<uint32_t> indexes;
                    child_indexes(node, indexes);

                    for(auto child: indexes) {
                        assert(child < nodes_.size());
                        visitor(nodes_[child]);
                    }
                }
            }
        };

        visitor(nodes_[0]);
    }

    AABB bounds() const { return bounds_; }

private:
    AABB calc_loose_bounds(Octree::Node& node) {
        /* Returns the loose bounds of the cell which is
         * twice the size, but with the same centre */

        AABB bounds = calc_bounds(node);
        auto centre = bounds.centre();

        float r = bounds.width();

        Vec3 diff(r, r, r);
        bounds.set_min(centre - diff);
        bounds.set_max(centre + diff);
        return bounds;
    }

    AABB calc_bounds(Octree::Node& node) {
        auto grid_width = ipow(2, node.level);
        auto cell_width = bounds_.max_dimension() / grid_width;

        Vec3 min(node.grid[0] * cell_width, node.grid[1] * cell_width, node.grid[2] * cell_width);
        min += bounds_.min();

        Vec3 max = min + Vec3(cell_width, cell_width, cell_width);
        return AABB(min, max);
    }

    uint32_t calc_index(uint8_t k, uint32_t x, uint32_t y, uint32_t z) {
        auto level_base = (k == 0) ? 0 : ipow(8, k - 1);
        auto level_width = ipow(2, k);
        auto idx = x + level_width * y + level_width * level_width * z;
        return level_base + idx;
    }

    void child_indexes(Octree::Node& node, std::vector<uint32_t>& indexes) {
        indexes.clear();

        for(uint8_t z = 0; z <= 1; ++z) {
            for(uint8_t y = 0; y <= 1; ++y) {
                for(uint8_t x = 0; x <= 1; ++x) {
                    indexes.push_back(calc_index(
                        node.level + 1,
                        2 * node.grid[0] + x,
                        2 * node.grid[1] + y,
                        2 * node.grid[2] + z
                    ));
                }
            }
        }
    }

    void grow() {
        // Prevent overflow (getting to this point would be crazy!)
        if(levels_ == std::numeric_limits<uint8_t>::max()) {
            return;
        }

        auto k = 0;
        if(!nodes_.empty()) {
            k = ++levels_;
        }

        auto level_grid_width = ipow(2, k);

        // Reserve space in one go
        auto new_node_count = ipow(level_grid_width, 3);
        nodes_.resize(nodes_.size() + new_node_count);

        for(auto z = 0; z < level_grid_width; ++z) {
            for(auto y = 0; y < level_grid_width; ++y) {
                for(auto x = 0; x < level_grid_width; ++x) {
                    auto idx = calc_index(k, x, y, z);
                    assert(idx < nodes_.size());
                    assert(idx >= 0);

                    auto& new_node = nodes_[idx];
                    new_node.grid[0] = x;
                    new_node.grid[1] = y;
                    new_node.grid[2] = z;
                    new_node.level = k;
                    new_node.data = new NodeData();
                }
            }
        }
    }

    TreeData* tree_data_ = nullptr;

    AABB bounds_;
    Vec3 centre_;

    uint8_t levels_ = 0;
    std::vector<Octree::Node> nodes_;
};

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
            /* FIXME: Need to be able to do this in bulk for performance */
            for(auto idx: p.second) {
                renderable->_indices().index(idx);
            }
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
