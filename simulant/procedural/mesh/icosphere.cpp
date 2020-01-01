#include "../../math/vec3.h"
#include "../../meshes/submesh.h"
#include "../../vertex_data.h"

#include "icosphere.h"

namespace smlt {
namespace procedural {
namespace mesh {

class IcoSphereCreator {
    struct TriangleIndices {
        int v1;
        int v2;
        int v3;

        TriangleIndices(int v1, int v2, int v3) {
            this->v1 = v1;
            this->v2 = v2;
            this->v3 = v3;
        }
    };

    int index_;
    std::unordered_map<int64_t, int> middle_point_index_cache_;
    std::vector<Vec3> positions_;

    // add vertex to mesh, fix position to be on unit sphere, return index
    int add_vertex(float x, float y, float z) {
        float length = std::sqrt(x * x + y * y + z * z);
        positions_.push_back(Vec3(x / length, y / length, z / length));
        return index_++;
    }

    // return index of point in the middle of p1 and p2
    int calc_middle_point(int p1, int p2) {
        // first check if we have it already
        bool firstIsSmaller = p1 < p2;
        int64_t smallerIndex = firstIsSmaller ? p1 : p2;
        int64_t greaterIndex = firstIsSmaller ? p2 : p1;
        auto key = (smallerIndex << 32) + greaterIndex;

        auto it = this->middle_point_index_cache_.find(key);
        if(it != this->middle_point_index_cache_.end()) {
            return it->second;
        }

        // not in cache, calculate it
        Vec3 point1 = this->positions_[p1];
        Vec3 point2 = this->positions_[p2];

        // add vertex makes sure point is on unit sphere
        auto i = add_vertex(
            (point1.x + point2.x) / 2.0f,
            (point1.y + point2.y) / 2.0f,
            (point1.z + point2.z) / 2.0f
        );

        // store it, return index
        middle_point_index_cache_.insert(std::make_pair(key, i));
        return i;
    }

public:
    void create(SubMesh* submesh, float diameter, int recursionLevel) {
        positions_.clear();
        middle_point_index_cache_.clear();
        index_ = 0;

        // create 12 vertices of a icosahedron
        auto t = (1.0f + sqrtf(5.0f)) / 2.0f;

        add_vertex(-1,  t,  0);
        add_vertex( 1,  t,  0);
        add_vertex(-1, -t,  0);
        add_vertex( 1, -t,  0);

        add_vertex( 0, -1,  t);
        add_vertex( 0,  1,  t);
        add_vertex( 0, -1, -t);
        add_vertex( 0,  1, -t);

        add_vertex( t,  0, -1);
        add_vertex( t,  0,  1);
        add_vertex(-t,  0, -1);
        add_vertex(-t,  0,  1);


        std::vector<TriangleIndices> faces;

        // create 20 triangles of the icosahedron

        // 5 faces around point 0
        faces.push_back(TriangleIndices(0, 11, 5));
        faces.push_back(TriangleIndices(0, 5, 1));
        faces.push_back(TriangleIndices(0, 1, 7));
        faces.push_back(TriangleIndices(0, 7, 10));
        faces.push_back(TriangleIndices(0, 10, 11));

        // 5 adjacent faces
        faces.push_back(TriangleIndices(1, 5, 9));
        faces.push_back(TriangleIndices(5, 11, 4));
        faces.push_back(TriangleIndices(11, 10, 2));
        faces.push_back(TriangleIndices(10, 7, 6));
        faces.push_back(TriangleIndices(7, 1, 8));

        // 5 faces around point 3
        faces.push_back(TriangleIndices(3, 9, 4));
        faces.push_back(TriangleIndices(3, 4, 2));
        faces.push_back(TriangleIndices(3, 2, 6));
        faces.push_back(TriangleIndices(3, 6, 8));
        faces.push_back(TriangleIndices(3, 8, 9));

        // 5 adjacent faces
        faces.push_back(TriangleIndices(4, 9, 5));
        faces.push_back(TriangleIndices(2, 4, 11));
        faces.push_back(TriangleIndices(6, 2, 10));
        faces.push_back(TriangleIndices(8, 6, 7));
        faces.push_back(TriangleIndices(9, 8, 1));

        // refine triangles
        for (int i = 0; i < recursionLevel; i++) {
            std::vector<TriangleIndices> faces2;

            for(auto& tri: faces) {
                // replace triangle by 4 triangles
                int a = calc_middle_point(tri.v1, tri.v2);
                int b = calc_middle_point(tri.v2, tri.v3);
                int c = calc_middle_point(tri.v3, tri.v1);

                faces2.push_back(TriangleIndices(tri.v1, a, c));
                faces2.push_back(TriangleIndices(tri.v2, b, a));
                faces2.push_back(TriangleIndices(tri.v3, c, b));
                faces2.push_back(TriangleIndices(a, b, c));
            }

            faces = faces2;
        }


        auto& vdata = submesh->vertex_data;
        auto start = submesh->vertex_data->count();

        auto& spec = vdata->vertex_specification();

        vdata->move_to_end();
        for(auto& pos: positions_) {
            /* Supply the position and scale */
            vdata->position(pos * diameter);
            if(spec.has_diffuse()) vdata->diffuse(smlt::Colour::WHITE);

            auto N = pos.normalized();
            // The normal is the position normalized as we generate around the origin
            if(spec.has_normals()) vdata->normal(N);

            Vec2 uv;
            uv.x = N.x / 2.0f + 0.5f;
            uv.y = N.y / 2.0f + 0.5f;

            if(spec.has_texcoord0()) vdata->tex_coord0(uv);
            if(spec.has_texcoord1()) vdata->tex_coord1(uv);

            vdata->move_next();
        }

        auto& idata = submesh->index_data;
        for(auto& tri: faces) {
            idata->index(start + tri.v1);
            idata->index(start + tri.v2);
            idata->index(start + tri.v3);
        }

        vdata->done();
        idata->done();
    }
};

void icosphere(SubMesh* out, float diameter, uint32_t subdivisions) {
    IcoSphereCreator creator;
    creator.create(out, diameter, subdivisions);
}

}
}
}
