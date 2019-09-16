#include "submesh.h"
#include "mesh.h"
#include "private.h"
#include "adjacency_info.h"

#include "../asset_manager.h"
#include "../window.h"

namespace smlt {

SubMesh::SubMesh(Mesh* parent, const std::string& name,
        MaterialID material, MeshArrangement arrangement, IndexType index_type):
    parent_(parent),
    name_(name),
    arrangement_(arrangement) {

    index_data_ = new IndexData(index_type);

    if(!material) {
        //Set the material to the default one (store the pointer to increment the ref-count)
        material = parent_->asset_manager().clone_default_material();
    }

    set_material_id(material);

    vrecalc_ = vertex_data->signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));
    irecalc_ = index_data->signal_update_complete().connect(std::bind(&SubMesh::_recalc_bounds, this));
}

const MaterialID SubMesh::material_id() const {
    return material_->id();
}

VertexData *SubMesh::get_vertex_data() const {
    return parent_->vertex_data.get();
}

IndexData* SubMesh::get_index_data() const {
    return index_data_;
}

void SubMesh::set_diffuse(const smlt::Colour& colour) {
    index_data->each([this, colour](uint32_t i) {
        vertex_data->move_to(i);
        vertex_data->diffuse(colour);
    });

    vertex_data->done();
}

void SubMesh::set_material_id(MaterialID mat) {
    auto old_material = (material_) ? material_->id() : MaterialID();

    if(old_material == mat) {
        // Don't do anything, don't fire the changed signal
        return;
    }

    if(mat) {
        // Set the material, store the shared_ptr to increment the ref count
        material_ = parent_->asset_manager().material(mat);
        if(!material_) {
            throw std::runtime_error("Tried to set invalid material on submesh");
        }

    } else {
        // Reset the material
        material_.reset();
    }

    signal_material_changed_(this, old_material, mat);
    parent_->signal_submesh_material_changed_(
        parent_->id(),
        this,
        old_material,
        mat
    );
}

void SubMesh::reverse_winding() {
    if(arrangement_ != MESH_ARRANGEMENT_TRIANGLES) {
        assert(0 && "Not implemented");
    }

    auto original = index_data->all();

    index_data->clear();
    for(uint32_t i = 0; i < original.size() / 3; ++i) {
        index_data->index(original[i * 3]);
        index_data->index(original[(i * 3) + 2]);
        index_data->index(original[(i * 3) + 1]);
    }
    index_data->done();
}

/**
 * @brief SubMesh::recalc_bounds
 *
 * Recalculate the bounds of the submesh. This involves interating over all of the
 * vertices that make up the submesh and so is potentially quite slow. This happens automatically
 * when index_data->done() is called.
 */
void SubMesh::_recalc_bounds() {
    //Set the min bounds to the max
    bounds_.set_min(Vec3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    ));

    //Set the max bounds to the min
    bounds_.set_max(Vec3(
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    ));

    if(!index_data_->count()) {
        bounds_ = AABB();
        return;
    }

    auto vertex_count = this->parent_->vertex_data_->count();

    // Store a raw-pointer for performance
    VertexData* vdata = vertex_data.get();
    auto& pos_attr = vdata->vertex_specification().position_attribute;

    /* Awful switching is for performance
       FIXME: Is there a better way to do this? I guess templated lambda or method
    */
    if(pos_attr == VERTEX_ATTRIBUTE_2F) {
        bounds_.set_max_z(0);
        bounds_.set_min_z(0);

        index_data_->each([&](uint32_t idx) {
            if(idx >= vertex_count) return; // Don't read outside the bounds
            auto pos = vdata->position_at<Vec2>(idx);
            if(pos->x < bounds_.min().x) bounds_.set_min_x(pos->x);
            if(pos->y < bounds_.min().y) bounds_.set_min_y(pos->y);
            if(pos->x > bounds_.max().x) bounds_.set_max_x(pos->x);
            if(pos->y > bounds_.max().y) bounds_.set_max_y(pos->y);
        });
    } else if(pos_attr == VERTEX_ATTRIBUTE_3F) {
        index_data_->each([&](uint32_t idx) {
            if(idx >= vertex_count) return; // Don't read outside the bounds

            auto pos = vdata->position_at<Vec3>(idx);
            if(pos->x < bounds_.min().x) bounds_.set_min_x(pos->x);
            if(pos->y < bounds_.min().y) bounds_.set_min_y(pos->y);
            if(pos->z < bounds_.min().z) bounds_.set_min_z(pos->z);

            if(pos->x > bounds_.max().x) bounds_.set_max_x(pos->x);
            if(pos->y > bounds_.max().y) bounds_.set_max_y(pos->y);
            if(pos->z > bounds_.max().z) bounds_.set_max_z(pos->z);
        });
    } else {
        index_data_->each([&](uint32_t idx) {
            assert(pos_attr == VERTEX_ATTRIBUTE_4F);

            if(idx >= vertex_count) return; // Don't read outside the bounds

            auto pos = vdata->position_at<Vec4>(idx);
            if(pos->x < bounds_.min().x) bounds_.set_min_x(pos->x);
            if(pos->y < bounds_.min().y) bounds_.set_min_y(pos->y);
            if(pos->z < bounds_.min().z) bounds_.set_min_z(pos->z);

            if(pos->x > bounds_.max().x) bounds_.set_max_x(pos->x);
            if(pos->y > bounds_.max().y) bounds_.set_max_y(pos->y);
            if(pos->z > bounds_.max().z) bounds_.set_max_z(pos->z);
        });
    }
}

void SubMesh::each_triangle(std::function<void (uint32_t, uint32_t, uint32_t)> cb) {
    if(arrangement_ == MESH_ARRANGEMENT_TRIANGLES) {
        assert(index_data_->count() % 3 == 0);
        for(uint32_t i = 0; i < index_data_->count(); i += 3) {
            cb(index_data_->at(i), index_data_->at(i + 1), index_data_->at(i + 2));
        }
    } else if(arrangement_ == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
        assert(index_data_->count() >= 3);

        for(uint32_t i = 2; i < index_data_->count(); ++i) {
            cb(
                index_data_->at(i - 2),
                index_data_->at(i - 1),
                index_data_->at(i)
            );
        }
    } else if(arrangement_ == MESH_ARRANGEMENT_LINES) {
        for(uint32_t i = 0; i < index_data_->count(); i += 2) {
            // Pass the 3rd index the same as the first for lines
            cb(index_data_->at(i), index_data_->at(i + 1), index_data_->at(i));
        }
    } else if(arrangement_ == MESH_ARRANGEMENT_LINE_STRIP) {
        assert(index_data_->count() >= 2);

        for(uint32_t i = 1; i < index_data_->count(); ++i) {
            // Pass the 3rd index the same as the first for lines
            cb(
                index_data_->at(i - 1),
                index_data_->at(i),
                index_data_->at(i - 1)
            );
        }
    } else if(arrangement_ == MESH_ARRANGEMENT_TRIANGLE_FAN) {
        auto hub = index_data_->at(0);
        for(uint32_t i = 2; i < index_data_->count(); ++i) {
            cb(
                hub, index_data_->at(i - 1), index_data_->at(i)
            );
        }
    }
}

/**
 * @brief SubMesh::generate_texture_coordinates_cube
 * @param texture - which texture unit to generate coordinates for
 *
 * Generates cube coordinates.
 *
 * WARNING: Will not duplicate vertices. If you share vertices across polygons this
 * will currently not give correct results. In future a "duplicate_vertices" argument
 * might be added.
 */
void SubMesh::generate_texture_coordinates_cube(uint32_t texture) {
    auto& vd = vertex_data;

    vd->move_to_start();
    for(uint16_t i = 0; i < vd->count(); ++i) {
        auto v = vd->normal_at<Vec3>(i); // Get the vertex normal

        // Work out the component with the largest value
        float absx = fabs(v->x);
        float absy = fabs(v->y);
        float absz = fabs(v->z);

        bool x = (absx > absy && absx > absz);
        bool y = (absy > absx && absy > absz);

        // Generate a orthagonal direction vector

        smlt::Vec3 dir(0, 0, 0);

        if(x) {
            dir.x = (v->x < 0) ? -1 : 1;
        } else if(y) {
            dir.y = (v->y < 0) ? -1 : 1;
        } else {
            dir.z = (v->z < 0) ? -1 : 1;
        }

        // Create a plane at the origin with the opposite direction
        Plane plane(-dir.x, -dir.y, -dir.z, 0.0f);

        smlt::Vec3 v1 = *vd->position_at<Vec3>(i) - bounds_.min();

        // Project the vertex position onto the plane
        smlt::Vec3 final = plane.project(v1);

        //Scale the final position on the plane by the size of the box
        // and subtract the lower corner so that everything is relative to 0,0,0
        // and scaled between 0 and 1
        final.x /= bounds_.width();
        final.y /= bounds_.height();
        final.z /= bounds_.depth();

        // Finally, offset the uv coordinate to the right 'square' of the cubic texture
        if(x) {
            if(dir.x >= 0) {
                final.x = 2.0 / 3.0 + (final.y / 3.0);
                final.y = 2.0 / 4.0 + (final.z / 4.0);
            } else {
                final.x = final.y / 3.0;
                final.y = 2.0 / 4.0 + (final.z / 4.0);
            }
        } else if(y) {
            if(dir.y >= 0) {
                final.x = 1.0 / 3.0 + (final.z / 3.0);
                final.y = 3.0 / 4.0 + (final.x / 4.0);
            } else {
                final.x = 1.0 / 3.0 + (final.z / 3.0);
                final.y = 1.0 / 4.0 + (final.x / 4.0);
            }
        } else {
            if(dir.z >= 0) {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = 2.0 / 4.0 + (final.y / 4.0);
            } else {
                final.x = 1.0 / 3.0 + (final.x / 3.0);
                final.y = (final.y / 4.0);
            }
        }

        switch(texture) {
            case 0: vd->tex_coord0(final.x, final.y);
                break;
            case 1: vd->tex_coord1(final.x, final.y);
                break;
            case 2: vd->tex_coord2(final.x, final.y);
                break;
            case 3: vd->tex_coord3(final.x, final.y);
                break;
            default:
                break;
        }
        vd->move_next();
    }

    vd->done();

}

SubMesh::~SubMesh() {
    vrecalc_.disconnect();
    irecalc_.disconnect();

    delete index_data_;
    index_data_ = nullptr;
}


}
