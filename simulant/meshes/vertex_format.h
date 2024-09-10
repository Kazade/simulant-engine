#pragma once

#include "../types.h"
#include "../utils/limited_vector.h"
#include <cstdint>

namespace smlt {

enum VertexAttributeArrangement : uint8_t {
    VERTEX_ATTRIBUTE_ARRANGEMENT_UNKNOWN = 0,
    VERTEX_ATTRIBUTE_ARRANGEMENT_ONE = 1,
    VERTEX_ATTRIBUTE_ARRANGEMENT_TWO = 2,
    VERTEX_ATTRIBUTE_ARRANGEMENT_THREE = 3,
    VERTEX_ATTRIBUTE_ARRANGEMENT_FOUR = 4,
    VERTEX_ATTRIBUTE_ARRANGEMENT_R = 1,
    VERTEX_ATTRIBUTE_ARRANGEMENT_RG = 2,
    VERTEX_ATTRIBUTE_ARRANGEMENT_RGB = 3,
    VERTEX_ATTRIBUTE_ARRANGEMENT_RGBA = 4,
    VERTEX_ATTRIBUTE_ARRANGEMENT_BGRA = 5,
    VERTEX_ATTRIBUTE_ARRANGEMENT_X = VERTEX_ATTRIBUTE_ARRANGEMENT_R,
    VERTEX_ATTRIBUTE_ARRANGEMENT_XY = VERTEX_ATTRIBUTE_ARRANGEMENT_RG,
    VERTEX_ATTRIBUTE_ARRANGEMENT_XYZ = VERTEX_ATTRIBUTE_ARRANGEMENT_RGB,
    VERTEX_ATTRIBUTE_ARRANGEMENT_XYZW = VERTEX_ATTRIBUTE_ARRANGEMENT_RGBA,
    VERTEX_ATTRIBUTE_ARRANGEMENT_MAX
};

enum VertexAttributeType : uint8_t {
    VERTEX_ATTRIBUTE_TYPE_BYTE = 0,
    VERTEX_ATTRIBUTE_TYPE_UNSIGNED_BYTE,
    VERTEX_ATTRIBUTE_TYPE_SHORT,
    VERTEX_ATTRIBUTE_TYPE_UNSIGNED_SHORT,
    VERTEX_ATTRIBUTE_TYPE_INT,
    VERTEX_ATTRIBUTE_TYPE_UNSIGNED_INT,
    VERTEX_ATTRIBUTE_TYPE_FLOAT,
    VERTEX_ATTRIBUTE_TYPE_MAX
};

enum VertexAttributeName : uint16_t {
    VERTEX_ATTRIBUTE_NAME_POSITION,
    VERTEX_ATTRIBUTE_NAME_COLOR,
    VERTEX_ATTRIBUTE_NAME_NORMAL,
    VERTEX_ATTRIBUTE_NAME_TEXCOORD_0,
    VERTEX_ATTRIBUTE_NAME_TEXCOORD_1,
    VERTEX_ATTRIBUTE_NAME_TEXCOORD_2,
    VERTEX_ATTRIBUTE_NAME_TEXCOORD_3,
    VERTEX_ATTRIBUTE_NAME_TANGENT,
    VERTEX_ATTRIBUTE_NAME_BITANGENT,
    VERTEX_ATTRIBUTE_NAME_GENERIC,
    VERTEX_ATTRIBUTE_NAME_MAX
};

struct VertexAttribute {
    VertexAttributeName name;
    VertexAttributeArrangement arrangement;
    VertexAttributeType type;

    std::size_t calc_size() const;
};

class VertexFormat {
    LimitedVector<VertexAttribute, 16> attributes;

    friend class VertexFormatBuilder;

public:
    std::size_t stride() const;
    smlt::optional<std::size_t> offset(VertexAttributeName name) const;
    smlt::optional<std::size_t> offset(std::size_t index) const;
    std::size_t attribute_count() const {
        return attributes.size();
    }

    const VertexAttribute& attribute(std::size_t index) const {
        return attributes[index];
    }

    bool operator==(const VertexFormat& rhs) const;
    bool operator!=(const VertexFormat& rhs) const;
};

class VertexFormatBuilder {
private:
    VertexFormatBuilder(const VertexFormat& fmt) :
        format_(fmt) {}

public:
    VertexFormatBuilder() = default;
    VertexFormatBuilder add(VertexAttributeName name,
                            VertexAttributeArrangement arrangement,
                            VertexAttributeType type) {

        VertexFormat fmt = format_;
        fmt.attributes.push_back({name, arrangement, type});
        return VertexFormatBuilder(fmt);
    }

    VertexFormat build() {
        return format_;
    }

    operator VertexFormat() {
        return format_;
    }

private:
    VertexFormat format_;
};

} // namespace smlt

namespace std {

template<>
struct hash<smlt::VertexFormat> {
    size_t operator()(const smlt::VertexFormat& spec) const {
        size_t seed = 0;

        for(std::size_t i = 0; i < spec.attribute_count(); ++i) {
            auto& attribute = spec.attribute(i);
            hash_combine(seed, (unsigned int)attribute.name);
            hash_combine(seed, (unsigned int)attribute.arrangement);
            hash_combine(seed, (unsigned int)attribute.type);
        }

        return seed;
    }
};

} // namespace std
