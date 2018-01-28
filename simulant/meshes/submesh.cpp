#include "submesh.h"
#include "mesh.h"
#include "private.h"
#include "adjacency_info.h"

#include "../resource_manager.h"
#include "../window.h"

namespace smlt {

SubMesh::SubMesh(Mesh* parent, const std::string& name,
        MaterialID material, MeshArrangement arrangement, IndexType index_type):
    parent_(parent),
    name_(name),
    arrangement_(arrangement) {

    index_data_ = new IndexData(index_type);
    index_data_->signal_update_complete().connect([this]() {
        this->index_buffer_dirty_ = true;
    });

    if(!material) {
        //Set the material to the default one (store the pointer to increment the ref-count)
        material = parent_->resource_manager().clone_default_material();
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

HardwareBuffer* SubMesh::vertex_buffer() const {
    return parent_->shared_vertex_buffer_.get();
}

void SubMesh::prepare_buffers() {
    auto* renderer = parent_->resource_manager().window->renderer.get();

    parent_->prepare_buffers();

    if(index_buffer_dirty_) {
        sync_buffer<IndexData, Renderer>(
            &index_buffer_, index_data_,
            renderer,
            HARDWARE_BUFFER_VERTEX_ARRAY_INDICES
        );
        index_buffer_dirty_ = false;
    }
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
        material_ = parent_->resource_manager().material(mat);
        if(!material_) {
            throw std::runtime_error("Tried to set invalid material on submesh");
        }

        material_change_connection_ = material_->signal_material_changed().connect(
            [=](MaterialID) {
                /* FIXME: This is a hack! We want material_changed event to take some kind of event
                 * structure so we can signal different types of event changes. Here we are signaling that
                 * the material passes changed so the material itself changed - not that it we changed from
                 * one material to another. Still, we need to make sure that we trigger the signal so that the
                 * render queue updates. */
                signal_material_changed_(this, material_->id(), material_->id());
            }
        );
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

    if(!index_data->count()) {
        bounds_ = AABB();
        return;
    }

    auto vertex_count = vertex_data->count();
    for(uint16_t idx: index_data->all()) {
        if(idx >= vertex_count) continue; // Don't read outside the bounds

        Vec3 pos = vertex_data->position_at<Vec3>(idx);
        if(pos.x < bounds_.min().x) bounds_.set_min_x(pos.x);
        if(pos.y < bounds_.min().y) bounds_.set_min_y(pos.y);
        if(pos.z < bounds_.min().z) bounds_.set_min_z(pos.z);

        if(pos.x > bounds_.max().x) bounds_.set_max_x(pos.x);
        if(pos.y > bounds_.max().y) bounds_.set_max_y(pos.y);
        if(pos.z > bounds_.max().z) bounds_.set_max_z(pos.z);
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

void SubMesh::set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass) {
    material_->pass(pass)->set_texture_unit(unit, tex);
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
        Vec3 v;
        vd->normal_at(i, v); // Get the vertex normal

        // Work out the component with the largest value
        float absx = fabs(v.x);
        float absy = fabs(v.y);
        float absz = fabs(v.z);

        bool x = (absx > absy && absx > absz);
        bool y = (absy > absx && absy > absz);

        // Generate a orthagonal direction vector

        smlt::Vec3 dir(0, 0, 0);

        if(x) {
            dir.x = (v.x < 0) ? -1 : 1;
        } else if(y) {
            dir.y = (v.y < 0) ? -1 : 1;
        } else {
            dir.z = (v.z < 0) ? -1 : 1;
        }

        // Create a plane at the origin with the opposite direction
        Plane plane(-dir.x, -dir.y, -dir.z, 0.0f);

        smlt::Vec3 v1 = vd->position_at<Vec3>(i) - bounds_.min();

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
