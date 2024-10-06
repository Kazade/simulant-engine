#pragma once

#include <cstdint>
#include "../../assets/material.h"

namespace smlt {

class IndexData;
class VertexRangeList;

namespace utils {

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
        const TriangleIterable* iterable;
        Triangle value;

        MeshArrangement arrangement;
        uint32_t idx = 0;
    }

    TriangleIterable(MeshArrangement arrangement,
                     const IndexData* indexes = nullptr,
                     const VertexRangeList* ranges = nullptr);

    iterator begin();
    iterator end();
};
}
}
