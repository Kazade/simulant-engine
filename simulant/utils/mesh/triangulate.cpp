#include <map>

#include "../../meshes/mesh.h"
#include "triangulate.h"

namespace smlt {

TriangleIterable::iterator& TriangleIterable::iterator::operator++() {
    return update(true);
}

TriangleIterable::iterator& TriangleIterable::iterator::update(bool increment) {
    if(!indexes_ && !ranges_ ||
       (indexes_ && indexes_->count() == 0 && !ranges_) ||
       (ranges_ && ranges_->size() == 0 && !indexes_) ||
       (indexes_ && ranges_ && ranges_->size() == 0 &&
        indexes_->count() == 0)) {
        // We're not iterating over anything
        ranges_ = nullptr;
        indexes_ = nullptr;
        return *this;
    }

    if(indexes_ && indexes_->count()) {
        // We're operating on indexes
        if(arrangement_ == MESH_ARRANGEMENT_TRIANGLES) {
            if(idx_ + 3 >= indexes_->count()) {
                // We're at the end
                idx_ = 0;
                indexes_ = nullptr;
                return *this;
            }

            idx_ += 3 * increment;

            value_.idx[0] = indexes_->at(idx_);
            value_.idx[1] = indexes_->at(idx_ + 1);
            value_.idx[2] = indexes_->at(idx_ + 2);

        } else if(arrangement_ == MESH_ARRANGEMENT_QUADS) {
            if(tri_counter_ % 2 == 0) {
                // Increment the index once we've returned both
                // triangles for this quad
                idx_ += 4 * increment;

                if(idx_ + 4 >= indexes_->count()) {
                    // We're at the end
                    idx_ = 0;
                    indexes_ = nullptr;
                    return *this;
                }

                value_.idx[0] = indexes_->at(idx_);
                value_.idx[1] = indexes_->at(idx_ + 1);
                value_.idx[2] = indexes_->at(idx_ + 2);

            } else {
                value_.idx[0] = indexes_->at(idx_);
                value_.idx[1] = indexes_->at(idx_ + 2);
                value_.idx[2] = indexes_->at(idx_ + 3);
            }
        } else if(arrangement_ == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
            if(idx_ + 3 >= indexes_->count()) {
                // We're at the end
                idx_ = 0;
                indexes_ = nullptr;
                return *this;
            }

            // We only increment one, because the last two
            // indexes will be reused for the next triangle
            idx_ += 1 * increment;

            // Alternate vertices as we iterate the strip
            value_.idx[0] = (tri_counter_ % 2 == 0) ? indexes_->at(idx_)
                                                    : indexes_->at(idx_ + 1);
            value_.idx[1] = (tri_counter_ % 2 == 0) ? indexes_->at(idx_ + 1)
                                                    : indexes_->at(idx_);
            value_.idx[2] = indexes_->at(idx_ + 2);

        } else {
            // We're operating on lines or line strips
            // so just make this == end()
            idx_ = 0;
            indexes_ = nullptr;
            ranges_ = nullptr;
            return *this;
        }

    } else if(ranges_ && ranges_->size()) {
        // We're operating on ranges
        auto current_range = ranges_->at(range_idx_);
        if(arrangement_ == MESH_ARRANGEMENT_TRIANGLES) {
            idx_ += 3 * increment;

            if(idx_ + 3 > current_range->count) {
                idx_ = 0;
                if(range_idx_ + 1 >= ranges_->size()) {
                    // We're at the end
                    ranges_ = nullptr;
                    range_idx_ = 0;
                    return *this;
                }
                current_range = ranges_->at(++range_idx_);
            }

            value_.idx[0] = current_range->start + idx_;
            value_.idx[1] = current_range->start + idx_ + 1;
            value_.idx[2] = current_range->start + idx_ + 2;
        } else if(arrangement_ == MESH_ARRANGEMENT_TRIANGLE_FAN) {

            if(idx_ + 2 > current_range->count) {
                idx_ = 1;
                if(range_idx_ + 1 >= ranges_->size()) {
                    // We're at the end
                    ranges_ = nullptr;
                    return *this;
                }
                current_range = ranges_->at(++range_idx_);
            }

            // We wanna start on idx 1 for the first triangle
            // because vertex 0 is always included
            if(tri_counter_ == 0) {
                idx_ = 1;
            }

            idx_ += 1 * increment;

            value_.idx[0] = current_range->start;
            value_.idx[1] = current_range->start + idx_;
            value_.idx[2] = current_range->start + idx_ + 1;
        } else if(arrangement_ == MESH_ARRANGEMENT_TRIANGLE_STRIP) {

        } else if(arrangement_ == MESH_ARRANGEMENT_QUADS) {
        }
    }

    ++tri_counter_;
    return *this;
}
}
