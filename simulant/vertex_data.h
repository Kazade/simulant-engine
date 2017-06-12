/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <vector>
#include "deps/kazsignal/kazsignal.h"

#include "generic/managed.h"
#include "colour.h"
#include "types.h"

namespace smlt {

class WindowBase;


uint32_t vertex_attribute_size(VertexAttribute attr);

enum VertexAttributeType {
    VERTEX_ATTRIBUTE_TYPE_EMPTY = 0,
    VERTEX_ATTRIBUTE_TYPE_POSITION,
    VERTEX_ATTRIBUTE_TYPE_NORMAL,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD0,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD1,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD2,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD3,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD4,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD5,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD6,
    VERTEX_ATTRIBUTE_TYPE_TEXCOORD7,
    VERTEX_ATTRIBUTE_TYPE_DIFFUSE,
    VERTEX_ATTRIBUTE_TYPE_SPECULAR
};

VertexAttribute attribute_for_type(VertexAttributeType type, const VertexSpecification& spec);

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
    void position(const Vec3& pos);
    void position(const Vec2& pos);
    void position(const Vec4& pos);

    template<typename T>
    T position_at(uint32_t idx) const;

    void normal(float x, float y, float z);
    void normal(const Vec3& n);

    void normal_at(int32_t idx, Vec3& out) {
        assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_3F);
        out = *((Vec3*) &data_[(idx * stride()) + vertex_specification_.normal_offset()]);
    }

    void normal_at(int32_t idx, Vec4& out) {
        assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_4F);
        out = *((Vec4*) &data_[(idx * stride()) + vertex_specification_.normal_offset()]);
    }

    void tex_coord0(float u, float v);
    void tex_coord0(float u, float v, float w);
    void tex_coord0(float x, float y, float z, float w);
    void tex_coord0(const Vec2& vec) { tex_coord0(vec.x, vec.y); }

    template<typename T>
    T texcoord0_at(uint32_t idx);

    template<typename T>
    T texcoord1_at(uint32_t idx) const;

    void tex_coord1(float u, float v);
    void tex_coord1(float u, float v, float w);
    void tex_coord1(float x, float y, float z, float w);
    void tex_coord1(const Vec2& vec) { tex_coord1(vec.x, vec.y); }

    void tex_coord2(float u, float v);
    void tex_coord2(float u, float v, float w);
    void tex_coord2(float x, float y, float z, float w);
    void tex_coord2(const Vec2& vec) { tex_coord2(vec.x, vec.y); }

    void tex_coord3(float u, float v);
    void tex_coord3(float u, float v, float w);
    void tex_coord3(float x, float y, float z, float w);
    void tex_coord3(const Vec2& vec) { tex_coord3(vec.x, vec.y); }

    void diffuse(float r, float g, float b, float a);
    void diffuse(const Colour& colour);

    void specular(float r, float g, float b, float a);
    void specular(const Colour& colour);

    uint32_t count() const { return vertex_count_; }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }
    bool empty() const { return data_.empty(); }

    const int32_t cursor_position() const { return cursor_position_; }

    inline uint32_t stride() const {
        return stride_;
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

    const VertexSpecification& specification() const { return vertex_specification_; }

private:
    VertexSpecification vertex_specification_;
    std::vector<uint8_t> data_;
    uint32_t vertex_count_ = 0;
    uint32_t stride_ = 0;

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

template<>
Vec2 VertexData::texcoord1_at<Vec2>(uint32_t idx) const;

template<>
Vec3 VertexData::texcoord1_at<Vec3>(uint32_t idx) const;

template<>
Vec4 VertexData::texcoord1_at<Vec4>(uint32_t idx) const;


typedef uint32_t Index;

class IndexData {
public:
    IndexData(IndexType type);

    void each(std::function<void (uint32_t)> cb);

    void reset();
    void clear() { indices_.clear(); }
    void resize(uint32_t size) { indices_.resize(size * stride(), 0); }
    void reserve(uint32_t size) { indices_.reserve(size * stride()); }

    std::vector<uint32_t> all();

    void index(uint32_t idx) {
        if(index_type_ == INDEX_TYPE_8_BIT) {
            if(idx > 255) throw std::out_of_range("Index too large");

            indices_.push_back(uint8_t(idx));
        } else if(index_type_ == INDEX_TYPE_16_BIT) {
            if(idx >= std::numeric_limits<uint16_t>::max()) throw std::out_of_range("Index too large");

            auto i = indices_.size();
            indices_.push_back(0);
            indices_.push_back(0);

            auto ptr = (uint16_t*) &indices_[i];
            *ptr = (uint16_t) idx;
        } else {
            auto i = indices_.size();

            indices_.push_back(0);
            indices_.push_back(0);
            indices_.push_back(0);
            indices_.push_back(0);

            auto ptr = (uint32_t*) &indices_[i];
            *ptr = (uint32_t) idx;
        }
    }

    void push(uint32_t idx) {
        index(idx);
    }

    void done();

    uint32_t at(const uint32_t i) {
        auto ptr = &indices_[i * stride()];

        switch(index_type_) {
            case INDEX_TYPE_8_BIT: return *ptr;
            case INDEX_TYPE_16_BIT: return *((uint16_t*) ptr);
            case INDEX_TYPE_32_BIT: return *((uint32_t*) ptr);
        default:
            throw std::logic_error("Invalid index type");
        }
    }

    uint32_t count() const {
        return indices_.size() / stride();
    }

    bool operator==(const IndexData& other) const {
        return this->indices_ == other.indices_;
    }

    bool operator!=(const IndexData& other) const {
        return !(*this == other);
    }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }

    const uint8_t* data() const { return &indices_[0]; }
    std::size_t data_size() const { return indices_.size() * sizeof(uint8_t); }

    uint32_t stride() const {
        switch(index_type_) {
            case INDEX_TYPE_8_BIT: return sizeof(uint8_t);
            case INDEX_TYPE_16_BIT: return sizeof(uint16_t);
            case INDEX_TYPE_32_BIT: return sizeof(uint32_t);
        default:
            throw std::logic_error("Invalid index type");
        }
    }

    IndexType index_type() const { return index_type_; }

private:

    IndexType index_type_;
    std::vector<uint8_t> indices_;

    sig::signal<void ()> signal_update_complete_;
};


}
