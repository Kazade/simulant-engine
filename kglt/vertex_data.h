#pragma once

#include <cstdint>
#include <vector>
#include "deps/kazsignal/kazsignal.h"

#include "generic/managed.h"
#include "colour.h"
#include "types.h"

namespace kglt {

class WindowBase;

enum VertexAttribute {
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_4F
};

uint32_t vertex_attribute_size(VertexAttribute attr);

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

struct VertexSpecification {
    static const VertexSpecification DEFAULT;
    static const VertexSpecification POSITION_ONLY;
    static const VertexSpecification POSITION_AND_DIFFUSE;

    VertexAttribute position_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute normal_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord0_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord1_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord2_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord3_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute diffuse_attribute = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute specular_attribute = VERTEX_ATTRIBUTE_NONE;

    VertexSpecification() = default;
    VertexSpecification(VertexAttribute position,
                        VertexAttribute normal=VERTEX_ATTRIBUTE_NONE,
                        VertexAttribute texcoord0=VERTEX_ATTRIBUTE_NONE,
                        VertexAttribute texcoord1=VERTEX_ATTRIBUTE_NONE,
                        VertexAttribute texcoord2=VERTEX_ATTRIBUTE_NONE,
                        VertexAttribute texcoord3=VERTEX_ATTRIBUTE_NONE,
                        VertexAttribute diffuse=VERTEX_ATTRIBUTE_NONE,
                        VertexAttribute specular=VERTEX_ATTRIBUTE_NONE):
        position_attribute(position),
        normal_attribute(normal),
        texcoord0_attribute(texcoord0),
        texcoord1_attribute(texcoord1),
        texcoord2_attribute(texcoord2),
        texcoord3_attribute(texcoord3),
        diffuse_attribute(diffuse),
        specular_attribute(specular) {


    }

    bool operator==(const VertexSpecification& rhs) const {
        return position_attribute == rhs.position_attribute &&
               normal_attribute == rhs.normal_attribute  &&
               texcoord0_attribute == rhs.texcoord0_attribute &&
               texcoord1_attribute == rhs.texcoord1_attribute &&
               texcoord2_attribute == rhs.texcoord2_attribute &&
               texcoord3_attribute == rhs.texcoord3_attribute &&
               diffuse_attribute == rhs.diffuse_attribute &&
               specular_attribute == rhs.specular_attribute;
    }

    bool operator!=(const VertexSpecification& rhs) const {
        return !(*this == rhs);
    }
};


class VertexData :
    public Managed<VertexData> {

public:
    VertexData(VertexSpecification vertex_specification);

    void reset(VertexSpecification vertex_specification);
    void clear();

    void move_to_start();
    void move_by(int32_t amount);
    void move_to(int32_t index);
    void move_to_end();
    uint16_t move_next();

    void done();

    void position(float x, float y, float z, float w);
    void position(float x, float y, float z);
    void position(float x, float y);
    void position(const kmVec3& pos);
    void position(const kmVec2& pos);
    void position(const kmVec4& pos);

    template<typename T>
    T position_at(uint32_t idx) const;

    void normal(float x, float y, float z);
    void normal(const kmVec3& n);

    void normal_at(int32_t idx, Vec3& out) {
        assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_3F);
        out = *((Vec3*) &data_[(idx * stride()) + normal_offset()]);
    }

    void normal_at(int32_t idx, Vec4& out) {
        assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_4F);
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

    bool has_positions() const { return bool(vertex_specification_.position_attribute); }
    bool has_normals() const { return bool(vertex_specification_.normal_attribute); }
    bool has_texcoord0() const { return bool(vertex_specification_.texcoord0_attribute); }
    bool has_texcoord1() const { return bool(vertex_specification_.texcoord1_attribute); }
    bool has_texcoord2() const { return bool(vertex_specification_.texcoord2_attribute); }
    bool has_texcoord3() const { return bool(vertex_specification_.texcoord3_attribute); }
    bool has_diffuse() const { return bool(vertex_specification_.diffuse_attribute); }
    bool has_specular() const { return bool(vertex_specification_.specular_attribute); }

    uint32_t count() const { return vertex_count_; }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }
    bool empty() const { return data_.empty(); }

    const int32_t cursor_position() const { return cursor_position_; }

    inline uint32_t stride() const {
        return stride_;
    }

    uint32_t position_offset(bool check=true) const {
        if(check && !has_positions()) { throw std::logic_error("No such attribute"); }
        return 0;
    }

    uint32_t normal_offset(bool check=true) const {
        if(check && !has_normals()) { throw std::logic_error("No such attribute"); }
        return vertex_attribute_size(vertex_specification_.position_attribute);
    }

    uint32_t texcoord0_offset(bool check=true) const {
        if(check && !has_texcoord0()) { throw std::logic_error("No such attribute"); }
        return normal_offset(false) + vertex_attribute_size(vertex_specification_.normal_attribute);
    }

    uint32_t texcoord1_offset(bool check=true) const {
        if(check && !has_texcoord1()) { throw std::logic_error("No such attribute"); }
        return texcoord0_offset(false) + vertex_attribute_size(vertex_specification_.texcoord0_attribute);
    }

    uint32_t texcoord2_offset(bool check=true) const {
        if(check && !has_texcoord2()) { throw std::logic_error("No such attribute"); }
        return texcoord1_offset(false) + vertex_attribute_size(vertex_specification_.texcoord1_attribute);
    }

    uint32_t texcoord3_offset(bool check=true) const {
        if(check && !has_texcoord3()) { throw std::logic_error("No such attribute"); }
        return texcoord2_offset(false) + vertex_attribute_size(vertex_specification_.texcoord2_attribute);
    }

    uint32_t diffuse_offset(bool check=true) const {
        if(check && !has_diffuse()) { throw std::logic_error("No such attribute"); }
        return texcoord3_offset(false) + vertex_attribute_size(vertex_specification_.texcoord3_attribute);
    }

    uint32_t specular_offset(bool check=true) const {
        if(check && !has_specular()) { throw std::logic_error("No such attribute"); }
        return diffuse_offset(false) + vertex_attribute_size(vertex_specification_.diffuse_attribute);
    }

    uint32_t copy_vertex_to_another(VertexData& out, uint32_t idx) {
        if(out.vertex_specification_ != this->vertex_specification_) {
            throw std::runtime_error("Cannot copy vertex as formats differ");
        }

        uint32_t start = idx * stride();
        uint32_t end = (idx + 1) * stride();

        out.data_.insert(out.data_.end(), data_.begin() + start, data_.begin() + end);
        out.vertex_count_++; //Increment the vertex count on the output

        // Return the index to the new vertex
        return out.count() - 1;
    }

    void interp_vertex(uint32_t source_idx, const VertexData& dest_state, uint32_t dest_idx, VertexData& out, uint32_t out_idx, float interp);
    uint8_t* data() { if(empty()) { return nullptr; } return &data_[0]; }
    uint32_t data_size() const { return data_.size(); }

    VertexAttribute attribute_for_type(VertexAttributeType type) const;

    void resize(uint32_t size) {
        data_.resize(size * stride(), 0);
        vertex_count_ = size;
    }

    VertexSpecification specification() const { return vertex_specification_; }

private:
    VertexSpecification vertex_specification_;
    uint32_t stride_ = 0;
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
        data_.resize((vertex_count_ + 1) * stride(), 0);
        vertex_count_++;
    }

    void position_checks();
    void recalc_attributes();
};

template<>
Vec2 VertexData::position_at<Vec2>(uint32_t idx) const;

template<>
Vec3 VertexData::position_at<Vec3>(uint32_t idx) const;

template<>
Vec4 VertexData::position_at<Vec4>(uint32_t idx) const;

template<>
Vec2 VertexData::texcoord0_at<Vec2>(uint32_t idx);

template<>
Vec3 VertexData::texcoord0_at<Vec3>(uint32_t idx);

template<>
Vec4 VertexData::texcoord0_at<Vec4>(uint32_t idx);

typedef uint32_t Index;

class IndexData {
public:
    IndexData();

    void reset();
    void clear() { indices_.clear(); }
    void resize(uint32_t size) { indices_.resize(size); }
    void reserve(uint32_t size) { indices_.reserve(size); }
    void index(Index idx) { indices_.push_back(idx); }
    void push(Index idx) { index(idx); }
    void done();
    Index at(const uint32_t i) { return indices_.at(i); }

    uint32_t count() const { return indices_.size(); }

    const std::vector<Index>& all() const { return indices_; }

    bool operator==(const IndexData& other) const {
        return this->indices_ == other.indices_;
    }

    bool operator!=(const IndexData& other) const {
        return !(*this == other);
    }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }

    Index* _raw_data() { return &indices_[0]; }
private:
    std::vector<Index> indices_;

    sig::signal<void ()> signal_update_complete_;
};


}
