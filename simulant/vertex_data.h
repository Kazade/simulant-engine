/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#if !defined(_MSC_VER)
#include <cstdint>
#else
#include <stdint.h>
#endif

#include <vector>

#include "color.h"
#include "generic/managed.h"
#include "generic/notifies_destruction.h"
#include "generic/uniquely_identifiable.h"
#include "meshes/index_buffer.h"
#include "meshes/vertex_format.h"
#include "signals/signal.h"
#include "types.h"

namespace smlt {

class Window;
class VertexBuffer;

typedef std::shared_ptr<VertexBuffer> VertexBufferPtr;
typedef std::shared_ptr<IndexBuffer> IndexBufferPtr;

/* What should we do after uploading the vertex
 * data to the GPU */
enum UploadFreeDataMode {
    UPLOAD_FREE_DATA_MODE_KEEP,
    UPLOAD_FREE_DATA_MODE_DISCARD
};

template<typename T>
class GPUUploadable {
public:
    virtual ~GPUUploadable() {}

    /* Control whether or not to free the vertex data
     * once it has been uploaded to a vertex buffer */
    void set_free_data_mode(UploadFreeDataMode mode) {
        free_data_mode_ = mode;
    }

    UploadFreeDataMode free_data_mode() const {
        return free_data_mode_;
    }

    /** Returns true if the vertex data was changed
     *  since the last upload to the GPU */
    bool is_dirty() const {
        return is_dirty_;
    }

    /* Mark the vertex data as requiring upload to the
     * GPU */
    void set_dirty(bool dirty) {
        is_dirty_ = dirty;
    }

    /** Return the attached vertex buffer, if any */
    const std::shared_ptr<T> gpu_buffer() const {
        return buffer_;
    }

protected:
    std::shared_ptr<T> buffer_;
    bool is_dirty_ = true;
    UploadFreeDataMode free_data_mode_ = UPLOAD_FREE_DATA_MODE_DISCARD;
};

class VertexData:
    public UniquelyIdentifiable<VertexData>,
    public NotifiesDestruction<VertexData>,
    public GPUUploadable<VertexBuffer> {

    friend class Renderer;

public:
    typedef std::shared_ptr<VertexData> ptr;

    VertexData(VertexFormat vertex_specification);
    virtual ~VertexData();

    VertexData(const VertexData& rhs) = delete;
    VertexData& operator=(const VertexData& rhs) = delete;

    void reset(VertexFormat vertex_specification);

    /* If release_memory is true, then allocated RAM
     * will be freed. This can be costly which is why it defaults
     * to false. */
    void clear(bool release_memory=false);

    void move_to_start();
    bool move_by(int32_t amount);
    bool move_to(int32_t index);
    void move_to_end();
    uint32_t move_next();

    void done();
    uint64_t last_updated() const;

    // /**
    //  * @brief attr_at
    //  * @param name
    //  * @param index
    //  * @return Returns a pointer to the specified attribute at the specified
    //  * position. If the attribute is not found, or the attribute is not the
    //  * correct type, nullptr is returned.
    //  */
    // template<typename T>
    // const T* attr_at(VertexAttributeName name, std::size_t index) const;

    /**
     * @brief attr_at
     * @param name
     * @param index
     * @return Returns a raw pointer to the specified attribute at the specified
     * position. If the attribute is not found a nullptr is returned.
     */
    const uint8_t* attr_at(VertexAttributeName name, std::size_t index) const {
        auto offset_maybe = vertex_specification_.offset(name);
        if(!offset_maybe) {
            return nullptr;
        }

        const uint8_t* target = &data_[index * stride_];
        target += offset_maybe.value_or(0);
        return target;
    }

    /**
     * @brief attr_as
     * @param name
     * @param index
     * @return Returns the specified attribute at the specified position as
     * the specified type. If the attribute is not found, or the attribute is
     * is not convertible to T, then an empty optional is returned.
     */
    template<typename T>
    optional<T> attr_as(VertexAttributeName name, std::size_t index) const {
        auto ret = attr_at(name, index);
        auto attr_maybe = vertex_specification_.attr(name);
        if(!ret || !attr_maybe) {
            return no_value;
        }

        return attr_maybe->unpack<T>(ret);
    }

    void position(float x, float y, float z, float w);
    void position(float x, float y, float z);
    void position(float x, float y);
    void position(const Vec3& pos);
    void position(const Vec2& pos);
    void position(const Vec4& pos);

    void normal(float x, float y, float z);
    void normal(const Vec3& n);

    void tex_coord0(float u, float v);
    void tex_coord0(float u, float v, float w);
    void tex_coord0(float x, float y, float z, float w);
    void tex_coord0(const Vec2& vec) { tex_coord0(vec.x, vec.y); }

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

    void color(float r, float g, float b, float a);
    void color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void color(const Color& color);

    uint32_t count() const { return vertex_count_; }

    sig::signal<void ()>& signal_update_complete() { return signal_update_complete_; }
    bool empty() const { return data_.empty(); }

    int32_t cursor_position() const { return cursor_position_; }
    int32_t cursor_offset() const { return cursor_position_ * stride_; }

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

    std::size_t extend(const VertexData& other) {
        if(vertex_specification_ != other.vertex_specification_) {
            S_ERROR("Tried to extend vertex data with incompatible data");
            return 0;
        }

        data_.insert(data_.end(), other.data_.begin(), other.data_.end());
        vertex_count_ += other.count();
        return count();
    }

    bool transform_by(const Mat4& transform) {
        auto pos_attr_maybe =
            vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION);

        if(!pos_attr_maybe) {
            return false;
        }

        auto pos_attr = pos_attr_maybe.value();

        if(pos_attr.arrangement != VERTEX_ATTR_ARRANGEMENT_TWO &&
           pos_attr.arrangement != VERTEX_ATTR_ARRANGEMENT_THREE &&
           pos_attr.arrangement != VERTEX_ATTR_ARRANGEMENT_FOUR) {
            S_ERROR(
                "Attempted to transform unsupported position attribute type");
            return false;
        }

        for(auto i = 0u; i < count(); ++i) {
            move_to(i);

            Vec4 pos = attr_as<Vec4>(VERTEX_ATTR_NAME_POSITION, i).value();
            pos = transform * pos;

            if(pos_attr.arrangement == VERTEX_ATTR_ARRANGEMENT_TWO) {
                position(pos.x, pos.y);
            } else if(pos_attr.arrangement == VERTEX_ATTR_ARRANGEMENT_THREE) {
                position(pos.x, pos.y, pos.z);
            } else if(pos_attr.arrangement == VERTEX_ATTR_ARRANGEMENT_FOUR) {
                position(pos);
            }
        }

        return true;
    }

    bool interp_vertex(uint32_t source_idx, const VertexData& dest_state, uint32_t dest_idx, VertexData& out, uint32_t out_idx, float interp);
    uint8_t* data() { if(empty()) { return nullptr; } return &data_[0]; }
    const uint8_t* data() const { if(empty()) { return nullptr; } return &data_[0]; }
    std::size_t data_size() const { return data_.size(); }

    VertexAttribute attribute_for_type(VertexAttributeType type) const;

    void reserve(uint32_t size);

    void resize(uint32_t size);

    const VertexFormat& vertex_specification() const { return vertex_specification_; }

    /* Clones this VertexData into another. The other data must have the same
     * specification and will be wiped if it contains vertices already.
     *
     * We don't do this with a traditional assignment operator as copying vertex data
     * is a costly and rare operation and so copying should be explicit
     *
     * Returns true on success, false otherwise.
    */
    bool clone_into(VertexData& other);

private:
    VertexFormat vertex_specification_;
    std::vector<uint8_t> data_;
    uint32_t vertex_count_ = 0;
    uint32_t stride_ = 0;
    int32_t cursor_position_ = 0;
    uint64_t last_updated_ = 0;

    void tex_coordX(uint8_t which, float u);
    void tex_coordX(uint8_t which, float u, float v);
    void tex_coordX(uint8_t which, float u, float v, float w);
    void tex_coordX(uint8_t which, float x, float y, float z, float w);
    void check_texcoord(uint8_t which);

    VertexAttribute attribute_from_type(VertexAttributeType type);

    sig::signal<void ()> signal_update_complete_;

    void push_back();

    void position_checks();
    void recalc_attributes();

    friend class VertexDataTest;
};

typedef std::shared_ptr<VertexData> VertexDataPtr;

typedef uint32_t Index;

class IndexData;

class IndexDataIterator {
private:
    friend class IndexData;

    IndexDataIterator(const IndexData* owner, int pos);

public:
    IndexDataIterator& operator++() {
        ptr_ += stride_;
        return *this;
    }

    bool operator==(const IndexDataIterator& rhs) const {
        return ptr_ == rhs.ptr_;
    }

    bool operator!=(const IndexDataIterator& rhs) const {
        return ptr_ != rhs.ptr_;
    }

    uint32_t operator*() const {
        switch(type_) {
        case INDEX_TYPE_8_BIT:
            return *((uint8_t*) ptr_);
        case INDEX_TYPE_16_BIT:
            return *((uint16_t*) ptr_);
        case INDEX_TYPE_32_BIT:
        default:
            return *((uint32_t*) ptr_);
        }
    }
private:
    const IndexData* owner_;
    IndexType type_;
    uint8_t stride_;
    const uint8_t* ptr_;
};

class IndexData:
    public UniquelyIdentifiable<IndexData>,
    public NotifiesDestruction<IndexData>,
    public GPUUploadable<IndexBuffer> {

    friend class IndexDataIterator;
public:
    typedef std::shared_ptr<IndexData> ptr;

    IndexData(IndexType type);

    IndexDataIterator begin() const {
        return IndexDataIterator(this, 0);
    }

    IndexDataIterator end() const {
        return IndexDataIterator(this, count());
    }

    void reset();
    void clear(bool release_memory=false);
    void resize(uint32_t size);
    void reserve(uint32_t size) { indices_.reserve(size * stride()); }

    std::vector<uint32_t> all();

    uint32_t min_index() const { return min_index_; }
    uint32_t max_index() const { return max_index_; }

    template<typename T, int BS>
    void _index(uint32_t* indexes, std::size_t count) {
        auto i = indices_.size();
        indices_.resize(i + (count * BS));

        uint32_t* idx = indexes;
        for(std::size_t j = 0; j < count; ++j) {
            min_index_ = std::min(min_index_, *idx);
            max_index_ = std::max(max_index_, *idx);

            auto ptr = (T*) &indices_[i];
            *ptr = (T) (*idx);
            ++idx;
            i += sizeof(T);
        }
    }

    void index(uint32_t* indexes, std::size_t count) {
        switch(index_type_) {
        case INDEX_TYPE_8_BIT:
            _index<uint8_t, 1>(indexes, count);
        break;
        case INDEX_TYPE_16_BIT:
            _index<uint16_t, 2>(indexes, count);
        break;
        case INDEX_TYPE_32_BIT:
            _index<uint32_t, 4>(indexes, count);
        break;
        default:
            break;
        }

        count_ = (uint32_t)(indices_.size() / stride_);
    }

    void index(uint32_t idx) {
        index(&idx, 1);
    }

    void done();
    uint64_t last_updated() const;

    uint32_t at(const uint32_t i) const {
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
        return count_;
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
        return stride_;
    }

    IndexType index_type() const { return index_type_; }

private:
    IndexType index_type_;
    std::vector<uint8_t> indices_;
    uint32_t stride_ = 0;
    uint32_t count_ = 0;
    uint64_t last_updated_ = 0;

    sig::signal<void ()> signal_update_complete_;

    /* For glDrawRangeElements */
    uint32_t min_index_ = ~0;
    uint32_t max_index_ = 0;
};

typedef IndexData::ptr IndexDataPtr;

}
