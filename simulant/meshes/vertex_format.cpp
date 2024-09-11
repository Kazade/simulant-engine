#include "vertex_format.h"

namespace smlt {

std::size_t VertexAttribute::calc_size() const {

    std::size_t size =
        (VERTEX_ATTR_ARRANGEMENT_BGRA) ? 4 : (std::size_t)arrangement;

    switch(type) {
        case VERTEX_ATTR_TYPE_BYTE:
        case VERTEX_ATTR_TYPE_UNSIGNED_BYTE:
            return size * 1;
        case VERTEX_ATTR_TYPE_SHORT:
        case VERTEX_ATTR_TYPE_UNSIGNED_SHORT:
            return size * 2;
        case VERTEX_ATTR_TYPE_INT:
        case VERTEX_ATTR_TYPE_UNSIGNED_INT:
        case VERTEX_ATTR_TYPE_FLOAT:
            return size * 4;
        default:
            return 0;
    }
}

std::size_t VertexFormat::stride() const {
    std::size_t ret = 0;
    for(auto& attr: attributes) {
        ret += attr.calc_size();
    }

    return ret;
}

smlt::optional<std::size_t>
    VertexFormat::offset(VertexAttributeName name) const {
    std::size_t ret = 0;
    for(auto& attr: attributes) {
        if(attr.name == name) {
            return ret;
        }
        ret += attr.calc_size();
    }

    // Not found
    return no_value;
}

std::size_t VertexFormat::offset(std::size_t index) const {
    assert(index < attributes.size() && "Index out of range");
    std::size_t ret = 0;
    for(std::size_t i = 0; i < index; ++i) {
        auto& a = attr(i);
        ret += a.calc_size();
    }

    return ret;
}

smlt::optional<VertexAttribute>
    VertexFormat::attr(VertexAttributeName name) const {
    for(auto& attr: attributes) {
        if(attr.name == name) {
            return attr;
        }
    }

    // Not found
    return no_value;
}

} // namespace smlt
