#pragma once

#include <cstdint>
#include <vector>

namespace smlt {

struct VertexRange {
    uint32_t start;
    uint32_t count;
};

class VertexRangeList {
private:
    std::vector<VertexRange> ranges_;

public:
    const VertexRange* at(std::size_t i) const {
        return &ranges_.at(i);
    }

    void add(uint32_t start, uint32_t count) {
        ranges_.push_back({start, count});
    }

    std::size_t size() const {
        return ranges_.size();
    }

    void clear() {
        ranges_.clear();
    }

    std::vector<VertexRange>::const_iterator begin() const {
        return ranges_.begin();
    }

    std::vector<VertexRange>::const_iterator end() const {
        return ranges_.end();
    }

    bool empty() const {
        return ranges_.empty();
    }
};

} // namespace smlt
