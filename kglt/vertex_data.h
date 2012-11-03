#ifndef VERTEX_DATA_H
#define VERTEX_DATA_H

#include <cstdint>
#include <vector>

#include "kazmath/vec3.h"
#include "kazmath/vec4.h"

#include "colour.h"

namespace kglt {

enum AttributeBitMask {
    BM_POSITIONS = 1,
    BM_NORMALS = 2,
    BM_TEXCOORD_0 = 4,
    BM_TEXCOORD_1 = 8,
    BM_TEXCOORD_2 = 16,
    BM_TEXCOORD_3 = 32,
    BM_TEXCOORD_4 = 64,
    BM_DIFFUSE = 128,
    BM_SPECULAR = 256
};

class VertexData {
public:
    VertexData();

    void set_texture_coordinate_dimensions(uint32_t coord_index, uint32_t count);

    void move_to_start();
    void move_to(uint16_t index);
    void move_to_end();
    void move_next();

    void done();

    void position(float x, float y, float z);
    void position(const kmVec3& pos);

    void normal(float x, float y, float z);
    void normal(const kmVec3& n);

    void tex_coord0(float u);
    void tex_coord0(float u, float v);
    void tex_coord0(float u, float v, float w);
    void tex_coord0(float x, float y, float z, float w);

    void tex_coord1(float u);
    void tex_coord1(float u, float v);
    void tex_coord1(float u, float v, float w);
    void tex_coord1(float x, float y, float z, float w);

    void tex_coord2(float u);
    void tex_coord2(float u, float v);
    void tex_coord2(float u, float v, float w);
    void tex_coord2(float x, float y, float z, float w);

    void tex_coord3(float u);
    void tex_coord3(float u, float v);
    void tex_coord3(float u, float v, float w);
    void tex_coord3(float x, float y, float z, float w);

    void tex_coord4(float u);
    void tex_coord4(float u, float v);
    void tex_coord4(float u, float v, float w);
    void tex_coord4(float x, float y, float z, float w);

    void diffuse(float r, float g, float b, float a);
    void diffuse(const Colour& colour);

    void specular(float r, float g, float b, float a);
    void specular(const Colour& colour);

    bool has_positions() const { return enabled_bitmask_ & BM_POSITIONS; }
    bool has_normals() const { return enabled_bitmask_ & BM_NORMALS; }
    bool has_texcoord0() const { return enabled_bitmask_ & BM_TEXCOORD_0; }
    bool has_texcoord1() const { return enabled_bitmask_ & BM_TEXCOORD_1; }
    bool has_texcoord2() const { return enabled_bitmask_ & BM_TEXCOORD_2; }
    bool has_texcoord3() const { return enabled_bitmask_ & BM_TEXCOORD_3; }
    bool has_texcoord4() const { return enabled_bitmask_ & BM_TEXCOORD_4; }
    bool has_diffuse() const { return enabled_bitmask_ & BM_DIFFUSE; }
    bool has_specular() const { return enabled_bitmask_ & BM_SPECULAR; }

    uint16_t count() const { return data_.size(); }

    bool operator==(const VertexData& other) const {
        return this->data_ == other.data_;
    }

    bool operator!=(const VertexData& other) const {
        return !(*this == other);
    }
private:
    int32_t enabled_bitmask_;
    uint16_t tex_coord_dimensions_[8];

    struct Vertex {
        kmVec3 position;
        kmVec3 normal;
        kmVec4 tex_coords[8];
        Colour diffuse_;
        Colour specular_;

        bool operator==(const Vertex& other) const {
            for(uint8_t i = 0; i < 8; ++i) {

                if(!kmVec4AreEqual(&this->tex_coords[i], &other.tex_coords[i])) {
                    return false;
                }
            }

            return kmVec3AreEqual(&this->position, &other.position) &&
                   kmVec3AreEqual(&this->normal, &other.normal) &&
                    this->diffuse_ == other.diffuse_ &&
                    this->specular_ == other.specular_;
        }
    };

    std::vector<Vertex> data_;
    int32_t cursor_position_;

    void check_or_add_attribute(AttributeBitMask attr);
};


class IndexData {
public:
    void clear() { indices_.clear(); }
    void reserve(uint16_t size) { indices_.reserve(size); }
    void index(uint16_t idx) { indices_.push_back(idx); }

    bool operator==(const IndexData& other) const {
        return this->indices_ == other.indices_;
    }

    bool operator!=(const IndexData& other) const {
        return !(*this == other);
    }

private:
    std::vector<uint16_t> indices_;
};


}

#endif // VERTEX_DATA_H
