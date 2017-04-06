#pragma once

#include <vector>
#include "../../types.h"
#include "../../generic/tri_octree.h"

namespace smlt {
namespace controllers {

struct Triangle {
    uint32_t index[3];
    Vec3 normal;

    group_id get_group(void*) const { return 0; }
    inline Vec3 get_vertex(uint32_t i, void* user_data) const {
        return ((Vec3*)user_data)[index[i]];
    }
};

typedef Octree<Triangle> RaycastOctree;


enum ColliderType {
    /* OOB collisiion detection */
    COLLIDER_TYPE_BOX,

     /* Useful for terrain meshes. Doesn't move or interact with other colliders but
     * allows for manual collider logic. */
    COLLIDER_TYPE_RAYCAST_ONLY
};


std::pair<Vec3, Vec3> calculate_bounds(const std::vector<Vec3>& vertices);


struct RaycastCollider {
    std::vector<Vec3> vertices;
    std::vector<Triangle> triangles;
    std::unique_ptr<RaycastOctree> octree_;

    void build_octree() {
        std::pair<Vec3, Vec3> min_max = calculate_bounds(vertices);
        octree_.reset(new RaycastOctree(min_max.first, min_max.second, 100, (void*)&vertices[0]));
        for(auto& triangle: triangles) {
            octree_->insert_triangle(triangle);
        }
    }

    std::pair<Vec3, bool> intersect_ray(const Vec3& start, const Vec3& direction, float* hit_distance=nullptr, Vec3* hit_normal=nullptr) {
        auto triangles = flatten(octree_->gather_triangles(octree_->find_nodes_intersecting_ray(start, direction)));

        Ray ray(start, direction);

        float closest_hit = std::numeric_limits<float>::max();
        Vec3 intersection, normal;
        bool intersects = false;

        for(auto& tri: triangles) {
            float dist = closest_hit;

            Vec3 intersect, n;
            bool hit = ray.intersects_triangle(
                vertices[tri.index[0]],
                vertices[tri.index[1]],
                vertices[tri.index[2]], &intersect, &n, &dist
            );

            if(hit && dist < closest_hit) {
                closest_hit = dist;
                intersection = intersect;
                intersects = true;
                normal = n;
            }
        }

        if(hit_distance) {
            *hit_distance = closest_hit;
        }

        if(hit_normal) {
            *hit_normal = normal;
        }

        return std::make_pair(intersection, intersects);
    }
};


}
}
