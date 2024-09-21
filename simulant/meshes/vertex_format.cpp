#include "vertex_format.h"

namespace smlt {

std::size_t VertexAttribute::component_count() const {
    if(arrangement == VERTEX_ATTR_ARRANGEMENT_BGRA) {
        return 4;
    } else {
        return (std::size_t)arrangement;
    }
}

std::size_t VertexAttribute::stride() const {

    std::size_t size = component_count();
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

    if(layout == VERTEX_LAYOUT_INTERLEAVE) {
        for(auto& attr: attributes) {
            if(attr.alignment) {
                ret = round_to_multiple(ret, attr.alignment);
            }

            ret += attr.stride();
        }

        // If the first attribute has an alignment, then the stride must
        // include the "gap" between this vertex and the next one
        if(attributes.size() && attributes[0].alignment) {
            ret = round_to_multiple(ret, attributes[0].alignment);
        }
    } else {
        for(auto& attr: attributes) {
            auto size = attr.stride();
            if(size > ret) {
                ret = size;
            }
        }
    }

    return ret;
}

smlt::optional<std::size_t>
    VertexFormat::offset(VertexAttributeName name) const {

    std::size_t ret = 0;
    for(auto& attr: attributes) {
        if(attr.name == name) {
            if(layout == VERTEX_LAYOUT_SEPARATE) {
                return 0;
            } else {
                return ret;
            }
        }
        ret += attr.stride();
    }

    // Not found
    return no_value;
}

std::size_t VertexFormat::offset(std::size_t index) const {
    assert(index < attributes.size() && "Index out of range");

    if(layout == VERTEX_LAYOUT_SEPARATE) {
        return 0;
    }

    std::size_t ret = 0;
    for(std::size_t i = 0; i < index; ++i) {
        auto& a = attr(i);
        ret += a.stride();
    }

    return ret;
}

std::size_t VertexFormat::attr_count(VertexAttributeName name) const {
    std::size_t ret = 0;
    for(auto& attr: attributes) {
        if(attr.name == name) {
            ret++;
        }
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

VertexFormat VertexFormat::standard() {
    return VertexFormatBuilder()
        .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_RGBA,
             VERTEX_ATTR_TYPE_UNSIGNED_BYTE)
        .add(VERTEX_ATTR_NAME_NORMAL, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .build();
}

VertexFormat VertexFormat::position_and_color() {
    return VertexFormatBuilder()
        .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_RGBA,
             VERTEX_ATTR_TYPE_UNSIGNED_BYTE)
        .build();
}

VertexFormat VertexFormat::position_and_texcoord() {
    return VertexFormatBuilder()
        .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
             VERTEX_ATTR_TYPE_FLOAT)
        .build();
}

VertexFormat VertexFormat::position_only() {
    return VertexFormatBuilder()
        .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .build();
}

} // namespace smlt
