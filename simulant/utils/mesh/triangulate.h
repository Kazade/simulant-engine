#pragma once

#include <cstdint>
#include "../../assets/material.h"

namespace smlt {

class IndexData;
class VertexRangeList;

struct Triangle {
    uint32_t idx[3];
};

/**
 * @brief The TriangleIterator class
 *
 * Given a list of indexes, ranges, or both, this class provides
 * a way to iterate over the triangles. Indexes will be iterated first
 * if both are provided.
 *
 * Line-based mesh arrangements will not return any values.
 */
class TriangleIterable {
public:
    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Triangle;
        using pointer = Triangle*;
        using reference = Triangle&;

    private:
        friend class TriangleIterable;

        const TriangleIterable* iterable_;
        Triangle value_;

        MeshArrangement arrangement_;

        // For indexes, this is the literal index into the index
        // list. For ranges this is the index into the current range
        uint32_t idx_ = 0;

        // Which range in the range list are we currently iterating?
        uint32_t range_idx_ = 0;

        // We keep track of how many triangles we've returned, because
        // for things like quads that changes how we generate the results
        uint32_t tri_counter_ = 0;

        const IndexData* indexes_ = nullptr;
        const VertexRangeList* ranges_ = nullptr;

        iterator& update(bool increment);

        iterator(MeshArrangement arrangement, const IndexData* indexes,
                 const VertexRangeList* ranges);

    public:
        iterator() = default;

        iterator& operator++();

        iterator operator++(int) {
            auto v = *this;
            ++(*this);
            return v;
        }

        bool operator==(const iterator& rhs) const {
            return indexes_ == rhs.indexes_ && ranges_ == rhs.ranges_ &&
                   range_idx_ == rhs.range_idx_ && idx_ == rhs.idx_;
        }

        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        Triangle& operator*() {
            return value_;
        }

        Triangle* operator->() {
            return &value_;
        }
    };

    TriangleIterable(MeshArrangement arrangement,
                     const IndexData* indexes = nullptr,
                     const VertexRangeList* ranges = nullptr) :
        arrangement_(arrangement), indexes_(indexes), ranges_(ranges) {

        assert(indexes || ranges);
    }

    iterator begin() {
        return iterator(arrangement_, indexes_, ranges_);
    }
    iterator end() {
        return iterator();
    }

private:
    MeshArrangement arrangement_;
    const IndexData* indexes_ = nullptr;
    const VertexRangeList* ranges_ = nullptr;
};
}
