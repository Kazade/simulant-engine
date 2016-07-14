#pragma once

#include <cstdint>
#include <vector>

#include "generic/managed.h"
#include "kazmath/vec2.h"
#include "kazmath/vec3.h"
#include "kazmath/vec4.h"
#include "buffer_object.h"
#include "colour.h"
#include "types.h"
#include <kazsignal/kazsignal.h>

namespace kglt {

class WindowBase;

enum VertexAttribute {
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_POSITION_2F = (1u << 1),
    VERTEX_ATTRIBUTE_POSITION_3F = (1u << 2),
    VERTEX_ATTRIBUTE_POSITION_4F = (1u << 3),

    VERTEX_ATTRIBUTE_NORMAL_3F = (1u << 4),
    VERTEX_ATTRIBUTE_NORMAL_4F = (1u << 5),

    VERTEX_ATTRIBUTE_TEXCOORD0_2F = (1u << 6),
    VERTEX_ATTRIBUTE_TEXCOORD0_3F = (1u << 7),
    VERTEX_ATTRIBUTE_TEXCOORD0_4F = (1u << 8),

    VERTEX_ATTRIBUTE_TEXCOORD1_2F = (1u << 9),
    VERTEX_ATTRIBUTE_TEXCOORD1_3F = (1u << 10),
    VERTEX_ATTRIBUTE_TEXCOORD1_4F = (1u << 11),

    VERTEX_ATTRIBUTE_TEXCOORD2_2F = (1u << 12),
    VERTEX_ATTRIBUTE_TEXCOORD2_3F = (1u << 13),
    VERTEX_ATTRIBUTE_TEXCOORD2_4F = (1u << 14),

    VERTEX_ATTRIBUTE_TEXCOORD3_2F = (1u << 15),
    VERTEX_ATTRIBUTE_TEXCOORD3_3F = (1u << 16),
    VERTEX_ATTRIBUTE_TEXCOORD3_4F = (1u << 17),

    VERTEX_ATTRIBUTE_DIFFUSE_3F = (1u << 18),
    VERTEX_ATTRIBUTE_DIFFUSE_4F = (1u << 19),

    VERTEX_ATTRIBUTE_SPECULAR_3F = (1u << 20),
    VERTEX_ATTRIBUTE_SPECULAR_4F = (1u << 21)
};

uint32_t vertex_attribute_size(VertexAttribute attr);

const std::set<VertexAttribute> POSITION_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_POSITION_2F,
    VERTEX_ATTRIBUTE_POSITION_3F,
    VERTEX_ATTRIBUTE_POSITION_4F
};

const std::set<VertexAttribute> NORMAL_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_NORMAL_3F,
    VERTEX_ATTRIBUTE_NORMAL_4F
};

const std::set<VertexAttribute> TEXCOORD0_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_TEXCOORD0_2F,
    VERTEX_ATTRIBUTE_TEXCOORD0_3F,
    VERTEX_ATTRIBUTE_TEXCOORD0_4F
};

const std::set<VertexAttribute> TEXCOORD1_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_TEXCOORD1_2F,
    VERTEX_ATTRIBUTE_TEXCOORD1_3F,
    VERTEX_ATTRIBUTE_TEXCOORD1_4F
};

const std::set<VertexAttribute> TEXCOORD2_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_TEXCOORD2_2F,
    VERTEX_ATTRIBUTE_TEXCOORD2_3F,
    VERTEX_ATTRIBUTE_TEXCOORD2_4F
};

const std::set<VertexAttribute> TEXCOORD3_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_TEXCOORD3_2F,
    VERTEX_ATTRIBUTE_TEXCOORD3_3F,
    VERTEX_ATTRIBUTE_TEXCOORD3_4F
};

const std::set<VertexAttribute> DIFFUSE_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_DIFFUSE_3F,
    VERTEX_ATTRIBUTE_DIFFUSE_4F
};

const std::set<VertexAttribute> SPECULAR_ATTRIBUTES = {
    VERTEX_ATTRIBUTE_SPECULAR_3F,
    VERTEX_ATTRIBUTE_SPECULAR_4F
};

enum VertexAttributeType {
    VERTEX_ATTRIBUTE_TYPE_EMPTY = 0,
    VERTEX_ATTRIBUTE_TYPE_POSITION,
    VERTEX_ATTRIBUTE_TYPE_NORMAL,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD0,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD1,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD2,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD3,
    VERTEX_ATTRIBUTE_TYPE_DIFFUSE,
    VERTEX_ATTRIBUTE_TYPE_SPECULAR
};


class VertexData :
    public Managed<VertexData> {

public:
    VertexData();
    VertexData(uint64_t attribute_mask);

    void reset(uint64_t attribute_mask);
    void clear();

    void move_to_start();
    void move_by(int32_t amount);
    void move_to(int32_t index);
    void move_to_end();
    uint16_t move_next();

    void done();

    void position(float x, float y, float z);
    void position(float x, float y);
    void position(const kmVec3& pos);
    void position(const kmVec2& pos);

    template<typename T>
    T position_at(uint32_t idx);

    void normal(float x, float y, float z);
    void normal(const kmVec3& n);

    void normal_at(int32_t idx, Vec3& out) {
        assert(normal_attribute_ == VERTEX_ATTRIBUTE_NORMAL_3F);
        out = *((Vec3*) &data_[(idx * stride()) + normal_offset()]);
    }

    void normal_at(int32_t idx, Vec4& out) {
        assert(normal_attribute_ == VERTEX_ATTRIBUTE_NORMAL_4F);
        out = *((Vec4*) &data_[(idx * stride()) + normal_offset()]);
    }

    void tex_coord0(float u, float v);
    void tex_coord0(float u, float v, float w);
    void tex_coord0(float x, float y, float z, float w);
    void tex_coord0(const kmVec2& vec) { tex_coord0(vec.x, vec.y); }

    template<typename T>
    T texcoord0_at(uint32_t idx);

    void tex_coord1(float u, float v);
    void tex_coord1(float u, float v, float w);
    void tex_coord1(float x, float y, float z, float w);
    void tex_coord1(const kmVec2& vec) { tex_coord1(vec.x, vec.y); }

    void tex_coord2(float u, float v);
    void tex_coord2(float u, float v, float w);
    void tex_coord2(float x, float y, float z, float w);
    void tex_coord2(const kmVec2& vec) { tex_coord2(vec.x, vec.y); }

    void tex_coord3(float u, float v);
    void tex_coord3(float u, float v, float w);
    void tex_coord3(float x, float y, float z, float w);
    void tex_coord3(const kmVec2& vec) { tex_coord3(vec.x, vec.y); }

    void diffuse(float r, float g, float b, float a);
    void diffuse(const Colour& colour);

    void specular(float r, float g, float b, float a);
    void specular(const Colour& colour);

    bool has_positions() const { return bool(position_attribute_); }
    bool has_normals() const { return bool(normal_attribute_); }
    bool has_texcoord0() const { return bool(texcoord0_attribute_); }
    bool has_texcoord1() const { return bool(texcoord1_attribute_); }
    bool has_texcoord2() const { return bool(texcoord2_attribute_); }
    bool has_texcoord3() const { return bool(texcoord3_attribute_); }
    bool has_diffuse() const { return bool(diffuse_attribute_); }
    bool has_specular() const { return bool(specular_attribute_); }

    uint32_t count() const { return vertex_count_; }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }
    bool empty() const { return data_.empty(); }

    const int32_t cursor_position() const { return cursor_position_; }

    uint32_t stride() const {
        return (
            vertex_attribute_size(position_attribute_) +
            vertex_attribute_size(normal_attribute_) +
            vertex_attribute_size(texcoord0_attribute_) +
            vertex_attribute_size(texcoord1_attribute_) +
            vertex_attribute_size(texcoord2_attribute_) +
            vertex_attribute_size(texcoord3_attribute_) +
            vertex_attribute_size(diffuse_attribute_) +
            vertex_attribute_size(specular_attribute_)
        );
    }

    uint32_t position_offset(bool check=true) const {
        if(check && !has_positions()) { throw std::logic_error("No such attribute"); }
        return 0;
    }

    uint32_t normal_offset(bool check=true) const {
        if(check && !has_normals()) { throw std::logic_error("No such attribute"); }
        return vertex_attribute_size(position_attribute_);
    }

    uint32_t texcoord0_offset(bool check=true) const {
        if(check && !has_texcoord0()) { throw std::logic_error("No such attribute"); }
        return normal_offset(false) + vertex_attribute_size(normal_attribute_);
    }

    uint32_t texcoord1_offset(bool check=true) const {
        if(check && !has_texcoord1()) { throw std::logic_error("No such attribute"); }
        return texcoord0_offset(false) + vertex_attribute_size(texcoord0_attribute_);
    }

    uint32_t texcoord2_offset(bool check=true) const {
        if(check && !has_texcoord2()) { throw std::logic_error("No such attribute"); }
        return texcoord1_offset(false) + vertex_attribute_size(texcoord1_attribute_);
    }

    uint32_t texcoord3_offset(bool check=true) const {
        if(check && !has_texcoord3()) { throw std::logic_error("No such attribute"); }
        return texcoord2_offset(false) + vertex_attribute_size(texcoord2_attribute_);
    }

    uint32_t diffuse_offset(bool check=true) const {
        if(check && !has_diffuse()) { throw std::logic_error("No such attribute"); }
        return texcoord3_offset(false) + vertex_attribute_size(texcoord3_attribute_);
    }

    uint32_t specular_offset(bool check=true) const {
        if(check && !has_specular()) { throw std::logic_error("No such attribute"); }
        return diffuse_offset(false) + vertex_attribute_size(diffuse_attribute_);
    }

    void copy_vertex_to_another(VertexData& out, uint32_t idx) {
        if(out.attribute_mask_ != this->attribute_mask_) {
            throw std::runtime_error("Cannot copy vertex as formats differ");
        }

        uint32_t start = idx * stride();
        uint32_t end = (idx + 1) * stride();

        out.data_.insert(out.data_.end(), data_.begin() + start, data_.begin() + end);
    }

    uint64_t vertex_format() const { return attribute_mask_; }

    uint8_t* data() { if(empty()) { return nullptr; } return &data_[0]; }
    uint32_t data_size() const { return data_.size(); }

    VertexAttribute attribute_for_type(VertexAttributeType type) const;
private:
    uint64_t attribute_mask_ = 0;

    VertexAttribute position_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute normal_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord0_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord1_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord2_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord3_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute diffuse_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute specular_attribute_ = VERTEX_ATTRIBUTE_NONE;

    std::vector<uint8_t> data_;
    uint32_t vertex_count_ = 0;

    int32_t cursor_position_ = 0;

    void tex_coordX(uint8_t which, float u);
    void tex_coordX(uint8_t which, float u, float v);
    void tex_coordX(uint8_t which, float u, float v, float w);
    void tex_coordX(uint8_t which, float x, float y, float z, float w);
    void check_texcoord(uint8_t which);

    VertexAttribute attribute_from_type(VertexAttributeType type);

    sig::signal<void ()> signal_update_complete_;

    void push_back() {
        for(uint32_t i = 0; i < stride(); ++i) { data_.push_back(0); }
        vertex_count_++;
    }

    void position_checks();
    void recalc_attributes();
};

template<>
Vec2 VertexData::position_at<Vec2>(uint32_t idx);

template<>
Vec3 VertexData::position_at<Vec3>(uint32_t idx);

template<>
Vec4 VertexData::position_at<Vec4>(uint32_t idx);

template<>
Vec2 VertexData::texcoord0_at<Vec2>(uint32_t idx);

template<>
Vec3 VertexData::texcoord0_at<Vec3>(uint32_t idx);

template<>
Vec4 VertexData::texcoord0_at<Vec4>(uint32_t idx);

typedef uint16_t Index;

class IndexData {
public:
    IndexData();

    void reset();
    void clear() { indices_.clear(); }
    void reserve(uint32_t size) { indices_.reserve(size); }
    void index(Index idx) { indices_.push_back(idx); }
    void push(Index idx) { index(idx); }
    void done();
    uint16_t at(const uint16_t i) { return indices_.at(i); }

    uint16_t count() const { return indices_.size(); }

    const std::vector<Index>& all() const { return indices_; }

    bool operator==(const IndexData& other) const {
        return this->indices_ == other.indices_;
    }

    bool operator!=(const IndexData& other) const {
        return !(*this == other);
    }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }

    uint16_t* _raw_data() { return &indices_[0]; }
private:
    std::vector<Index> indices_;

    sig::signal<void ()> signal_update_complete_;
};


}
