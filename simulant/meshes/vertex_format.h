#pragma once

#include "../types.h"
#include "../utils/limited_vector.h"
#include <cstdint>

namespace smlt {

enum VertexLayout {
    VERTEX_LAYOUT_INTERLEAVE,
    VERTEX_LAYOUT_SEPARATE
};

enum VertexAttributeArrangement : uint8_t {
    VERTEX_ATTR_ARRANGEMENT_INVALID = 0,
    VERTEX_ATTR_ARRANGEMENT_ONE = 1,
    VERTEX_ATTR_ARRANGEMENT_TWO = 2,
    VERTEX_ATTR_ARRANGEMENT_THREE = 3,
    VERTEX_ATTR_ARRANGEMENT_FOUR = 4,
    VERTEX_ATTR_ARRANGEMENT_R = 1,
    VERTEX_ATTR_ARRANGEMENT_RG = 2,
    VERTEX_ATTR_ARRANGEMENT_RGB = 3,
    VERTEX_ATTR_ARRANGEMENT_RGBA = 4,
    VERTEX_ATTR_ARRANGEMENT_BGRA = 5,
    VERTEX_ATTR_ARRANGEMENT_X = VERTEX_ATTR_ARRANGEMENT_R,
    VERTEX_ATTR_ARRANGEMENT_XY = VERTEX_ATTR_ARRANGEMENT_RG,
    VERTEX_ATTR_ARRANGEMENT_XYZ = VERTEX_ATTR_ARRANGEMENT_RGB,
    VERTEX_ATTR_ARRANGEMENT_XYZW = VERTEX_ATTR_ARRANGEMENT_RGBA,
    VERTEX_ATTR_ARRANGEMENT_MAX
};

enum VertexAttributeType : uint8_t {
    VERTEX_ATTR_TYPE_INVALID = 0,
    VERTEX_ATTR_TYPE_BYTE,
    VERTEX_ATTR_TYPE_UNSIGNED_BYTE,
    VERTEX_ATTR_TYPE_SHORT,
    VERTEX_ATTR_TYPE_UNSIGNED_SHORT,
    VERTEX_ATTR_TYPE_INT,
    VERTEX_ATTR_TYPE_UNSIGNED_INT,
    VERTEX_ATTR_TYPE_FLOAT,
    VERTEX_ATTR_TYPE_MAX
};

enum VertexAttributeName : uint16_t {
    VERTEX_ATTR_NAME_INVALID = 0,
    VERTEX_ATTR_NAME_POSITION,
    VERTEX_ATTR_NAME_COLOR,
    VERTEX_ATTR_NAME_NORMAL,
    VERTEX_ATTR_NAME_TEXCOORD_0,
    VERTEX_ATTR_NAME_TEXCOORD_1,
    VERTEX_ATTR_NAME_TEXCOORD_2,
    VERTEX_ATTR_NAME_TEXCOORD_3,
    VERTEX_ATTR_NAME_TANGENT,
    VERTEX_ATTR_NAME_BITANGENT,
    VERTEX_ATTR_NAME_GENERIC,
    VERTEX_ATTR_NAME_MAX
};

struct VertexAttribute {
private:
    template<typename T>
    struct UnpackID {
        typedef T type;
    };

    optional<Vec4> unpack_byte_to_vec4(const uint8_t* ptr,
                                       bool normalize) const {
        switch(arrangement) {
            case VERTEX_ATTR_ARRANGEMENT_BGRA:
                return (normalize) ? Vec4(ptr[2] / 255.0f, ptr[1] / 255.0f,
                                          ptr[0] / 255.0f, ptr[3] / 255.0f)
                                   : Vec4(ptr[2], ptr[1], ptr[0], ptr[3]);
            case VERTEX_ATTR_ARRANGEMENT_X:
                return (normalize) ? Vec4(ptr[0] / 255.0f, 0, 0, 1)
                                   : Vec4(ptr[0], 0, 0, 1);

            case VERTEX_ATTR_ARRANGEMENT_XY:
                return (normalize)
                           ? Vec4(ptr[0] / 255.0f, ptr[1] / 255.0f, 0, 1)
                           : Vec4(ptr[0], ptr[1], 0, 1);

            case VERTEX_ATTR_ARRANGEMENT_XYZ:
                return (normalize) ? Vec4(ptr[0] / 255.0f, ptr[1] / 255.0f,
                                          ptr[2] / 255.0f, 1)
                                   : Vec4(ptr[0], ptr[1], ptr[2], 1);
            case VERTEX_ATTR_ARRANGEMENT_XYZW:
                return (normalize) ? Vec4(ptr[0] / 255.0f, ptr[1] / 255.0f,
                                          ptr[2] / 255.0f, ptr[3] / 255.0f)
                                   : Vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
            default:
                return no_value;
        }
    }

    optional<Vec4> unpack_short_to_vec4(const uint8_t* ptr,
                                        bool normalize) const {
        switch(arrangement) {
            case VERTEX_ATTR_ARRANGEMENT_BGRA:
                return (normalize) ? Vec4(ptr[2] / 65535.0f, ptr[1] / 65535.0f,
                                          ptr[0] / 65535.0f, ptr[3] / 65535.0f)
                                   : Vec4(ptr[2], ptr[1], ptr[0], ptr[3]);
            case VERTEX_ATTR_ARRANGEMENT_X:
                return (normalize) ? Vec4(ptr[0] / 65535.0f, 0, 0, 1)
                                   : Vec4(ptr[0], 0, 0, 1);

            case VERTEX_ATTR_ARRANGEMENT_XY:
                return (normalize)
                           ? Vec4(ptr[0] / 65535.0f, ptr[1] / 65535.0f, 0, 1)
                           : Vec4(ptr[0], ptr[1], 0, 1);
            case VERTEX_ATTR_ARRANGEMENT_XYZ:
                return (normalize) ? Vec4(ptr[0] / 65535.0f, ptr[1] / 65535.0f,
                                          ptr[2] / 65535.0f, 1)
                                   : Vec4(ptr[0], ptr[1], ptr[2], 1);
            case VERTEX_ATTR_ARRANGEMENT_XYZW:
                return (normalize) ? Vec4(ptr[0] / 65535.0f, ptr[1] / 65535.0f,
                                          ptr[2] / 65535.0f, ptr[3] / 65535.0f)
                                   : Vec4(ptr[0], ptr[1], ptr[2], ptr[3]);

            default:
                return no_value;
        }
    }

    optional<Vec4> unpack_int_to_vec4(const uint8_t* ptr,
                                      bool normalize) const {
        switch(arrangement) {
            case VERTEX_ATTR_ARRANGEMENT_BGRA:
                return (normalize)
                           ? Vec4(*reinterpret_cast<const int*>(ptr + 8) /
                                      65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 4) /
                                      65535.0f,
                                  *reinterpret_cast<const int*>(ptr) / 65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 12) /
                                      65535.0f)
                           : Vec4(*reinterpret_cast<const int*>(ptr + 8),
                                  *reinterpret_cast<const int*>(ptr + 4),
                                  *reinterpret_cast<const int*>(ptr),
                                  *reinterpret_cast<const int*>(ptr + 12));
            case VERTEX_ATTR_ARRANGEMENT_X:
                return (normalize)
                           ? Vec4(*reinterpret_cast<const int*>(ptr) / 65535.0f,
                                  0, 0, 1)
                           : Vec4(*reinterpret_cast<const int*>(ptr), 0, 0, 1);
            case VERTEX_ATTR_ARRANGEMENT_XY:
                return (normalize)
                           ? Vec4(*reinterpret_cast<const int*>(ptr) / 65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 4) /
                                      65535.0f,
                                  0, 1)
                           : Vec4(*reinterpret_cast<const int*>(ptr),
                                  *reinterpret_cast<const int*>(ptr + 4), 0, 1);
            case VERTEX_ATTR_ARRANGEMENT_XYZ:
                return (normalize)
                           ? Vec4(*reinterpret_cast<const int*>(ptr) / 65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 4) /
                                      65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 8) /
                                      65535.0f,
                                  1)
                           : Vec4(*reinterpret_cast<const int*>(ptr),
                                  *reinterpret_cast<const int*>(ptr + 4),
                                  *reinterpret_cast<const int*>(ptr + 8), 1);
            case VERTEX_ATTR_ARRANGEMENT_RGBA:
                return (normalize)
                           ? Vec4(*reinterpret_cast<const int*>(ptr) / 65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 4) /
                                      65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 8) /
                                      65535.0f,
                                  *reinterpret_cast<const int*>(ptr + 12) /
                                      65535.0f)
                           : Vec4(*reinterpret_cast<const int*>(ptr),
                                  *reinterpret_cast<const int*>(ptr + 4),
                                  *reinterpret_cast<const int*>(ptr + 8),
                                  *reinterpret_cast<const int*>(ptr + 12));
            default:
                return no_value;
        }
    }

    optional<Vec4> unpack_float_to_vec4(const uint8_t* ptr,
                                        bool normalize) const {
        _S_UNUSED(normalize);
        switch(arrangement) {
            case VERTEX_ATTR_ARRANGEMENT_BGRA:
                return Vec4(*reinterpret_cast<const float*>(ptr + 8),
                            *reinterpret_cast<const float*>(ptr + 4),
                            *reinterpret_cast<const float*>(ptr),
                            *reinterpret_cast<const float*>(ptr + 12));
            case VERTEX_ATTR_ARRANGEMENT_XYZW:
                return Vec4(*reinterpret_cast<const float*>(ptr),
                            *reinterpret_cast<const float*>(ptr + 4),
                            *reinterpret_cast<const float*>(ptr + 8),
                            *reinterpret_cast<const float*>(ptr + 12));
            case VERTEX_ATTR_ARRANGEMENT_XYZ:
                return Vec4(*reinterpret_cast<const float*>(ptr),
                            *reinterpret_cast<const float*>(ptr + 4),
                            *reinterpret_cast<const float*>(ptr + 8), 1);
            case VERTEX_ATTR_ARRANGEMENT_XY:
                return Vec4(*reinterpret_cast<const float*>(ptr),
                            *reinterpret_cast<const float*>(ptr + 4), 0, 1);
            case VERTEX_ATTR_ARRANGEMENT_X:
                return Vec4(*reinterpret_cast<const float*>(ptr), 0, 0, 1);
            default:
                return no_value;
        }
    }

    optional<Vec4> unpack(UnpackID<Vec4>, const uint8_t* ptr, bool normalize) {
        switch(type) {
            case VERTEX_ATTR_TYPE_UNSIGNED_BYTE:
            case VERTEX_ATTR_TYPE_BYTE:
                return unpack_byte_to_vec4(ptr, normalize);
            case VERTEX_ATTR_TYPE_UNSIGNED_SHORT:
            case VERTEX_ATTR_TYPE_SHORT:
                return unpack_short_to_vec4(ptr, normalize);
            case VERTEX_ATTR_TYPE_FLOAT:
                return unpack_float_to_vec4(ptr, normalize);
            case VERTEX_ATTR_TYPE_INT:
            case VERTEX_ATTR_TYPE_UNSIGNED_INT:
                return unpack_int_to_vec4(ptr, normalize);
            default:
                return no_value;
        }
    }

    optional<Vec3> unpack(UnpackID<Vec3>, const uint8_t* ptr, bool normalize) {
        auto ret = unpack(UnpackID<Vec4>(), ptr, normalize);
        if(ret) {
            return ret->xyz();
        } else {
            return no_value;
        }
    }

    optional<Vec2> unpack(UnpackID<Vec2>, const uint8_t* ptr, bool normalize) {
        auto ret = unpack(UnpackID<Vec4>(), ptr, normalize);
        if(ret) {
            return ret->xy();
        } else {
            return no_value;
        }
    }

    optional<Color> unpack(UnpackID<Color>, const uint8_t* ptr,
                           bool normalize) {
        _S_UNUSED(normalize);
        auto ret = unpack(UnpackID<Vec4>(), ptr, true);
        if(ret) {
            return Color(ret->x, ret->y, ret->z, ret->w);
        } else {
            return no_value;
        }
    }

public:
    VertexAttributeName name = VERTEX_ATTR_NAME_INVALID;
    VertexAttributeArrangement arrangement = VERTEX_ATTR_ARRANGEMENT_INVALID;
    VertexAttributeType type = VERTEX_ATTR_TYPE_INVALID;
    std::size_t alignment = 0;

    std::size_t stride() const;
    std::size_t component_count() const;

    template<typename T>
    optional<T> unpack(const uint8_t* ptr, bool normalize = false) {
        return unpack(UnpackID<T>(), ptr, normalize);
    }

    bool operator==(const VertexAttribute& rhs) const {
        return name == rhs.name && arrangement == rhs.arrangement &&
               type == rhs.type;
    }

    bool operator!=(const VertexAttribute& rhs) const {
        return !(*this == rhs);
    }
};

class VertexFormat {
    VertexLayout layout = VERTEX_LAYOUT_INTERLEAVE;
    LimitedVector<VertexAttribute, 16> attributes;

    friend class VertexFormatBuilder;

public:
    VertexFormat(VertexLayout layout = VERTEX_LAYOUT_INTERLEAVE) :
        layout(layout) {}

    /**
     * @brief stride
     * @return In the case of VERTEX_LAYOUT_INTERLEAVE, this returns the stride
     * of each vertex including alignment. In the case of In the case of
     * VERTEX_LAYOUT_SEPARATE this returns the largest attribute stride.
     */
    std::size_t stride() const;

    smlt::optional<std::size_t> offset(VertexAttributeName name) const;
    std::size_t offset(std::size_t index) const;
    std::size_t attr_count() const {
        return attributes.size();
    }

    const VertexAttribute& attr(std::size_t index) const {
        return attributes[index];
    }

    std::size_t attr_count(VertexAttributeName name) const;

    smlt::optional<VertexAttribute> attr(VertexAttributeName name) const;

    bool operator==(const VertexFormat& rhs) const {
        for(std::size_t i = 0; i < attr_count(); ++i) {
            if(attr(i) != rhs.attr(i)) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const VertexFormat& rhs) const {
        return !(*this == rhs);
    }

    static VertexFormat standard();
    static VertexFormat position_and_color();
    static VertexFormat position_and_texcoord();
    static VertexFormat position_only();
};

class VertexFormatBuilder {
private:
    VertexFormatBuilder(const VertexFormat& fmt) :
        format_(fmt) {}

public:
    VertexFormatBuilder(VertexLayout layout = VERTEX_LAYOUT_INTERLEAVE) :
        format_(layout) {}

    VertexFormatBuilder add(VertexAttributeName name,
                            VertexAttributeArrangement arrangement,
                            VertexAttributeType type,
                            std::size_t alignment = 0) {

        VertexFormat fmt = format_;
        fmt.attributes.push_back({name, arrangement, type, alignment});
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

        for(std::size_t i = 0; i < spec.attr_count(); ++i) {
            auto& attribute = spec.attr(i);
            hash_combine(seed, (unsigned int)attribute.name);
            hash_combine(seed, (unsigned int)attribute.arrangement);
            hash_combine(seed, (unsigned int)attribute.type);
        }

        return seed;
    }
};

} // namespace std
