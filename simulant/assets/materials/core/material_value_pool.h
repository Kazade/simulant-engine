#pragma once

#include <vector>

#include "../../../generic/allocators/dynamic_aligned_allocator.h"
#include "../property_value.h"

namespace smlt {

/* We have to use a 16 byte alignment, because
 * the destructor pointer takes up 8-bytes on 64 bit systems.
 * It would be nice not to be so wasteful, but in the grand scheme
 * of things we're saving a tonne of memory by using a flyweight pattern
 * anyway, so we'll still be saving space overall. */
static constexpr int alignment = 16;

struct BlockHeader {
    MaterialPropertyType type;
    uint16_t refcount = 0;
    void (*destructor)(void* ptr);
};

/* We need to be able to defrag the material value pool, so we return pointers
   which are ref-counted to tell if values are still used and maintain our own
   copies in the pool so we can update pointers if memory shifts around */
class MaterialPropertyValuePointer {
    std::shared_ptr<uint8_t*> data_;

    friend class MaterialValuePool;

    MaterialPropertyValuePointer(const std::shared_ptr<uint8_t*>& data) :
        data_(data) {
        if(data_ && *data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            ++header->refcount;
        }
    }

    void increase_refcount() {
        if(data_ && *data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            ++header->refcount;
        }
    }

    void decrease_refcount() {
        if(data_ && *data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            assert(header->refcount > 0);

            --header->refcount;

            if(header->refcount == 0) {
                header->destructor(*data_ + alignment);
                *data_ = nullptr;
            }
        }
    }

public:
    MaterialPropertyValuePointer() = default;
    MaterialPropertyValuePointer(const MaterialPropertyValuePointer& other) {
        data_ = other.data_;

        if(data_ && *data_) {
            increase_refcount();
        }
    }

    MaterialPropertyValuePointer&
        operator=(const MaterialPropertyValuePointer& other) {
        if(this == &other) {
            return *this;
        }

        data_ = other.data_;
        if(data_ && *data_) {
            increase_refcount();
        }
        return *this;
    }

    ~MaterialPropertyValuePointer() {
        if(data_ && *data_) {
            decrease_refcount();
        }
    }

    template<typename T>
    T* get() {
        return reinterpret_cast<T*>((*data_) + alignment);
    }

    template<typename T>
    const T* get() const {
        return reinterpret_cast<const T*>((*data_) + alignment);
    }

    // Warning: don't call this on a null pointer
    MaterialPropertyType type() const {
        BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
        return header->type;
    }

    std::size_t refcount() const {
        if(!data_ || !*data_) {
            return 0;
        }

        BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
        return header->refcount;
    }

    void reset() {
        if(data_ && *data_) {
            decrease_refcount();
        }

        data_.reset();
    }

    operator bool() {
        return data_ && *data_;
    }
};

#define _CHECK_ALIGNMENT(type)                                                 \
    static_assert((alignment % alignof(type)) == 0 ||                          \
                      (alignment % alignof(type)) == alignment,                \
                  "Invalid alignment for type")

class MaterialValuePool {
public:
    static_assert(sizeof(BlockHeader) <= alignment, "Alignment is too small");

    _CHECK_ALIGNMENT(float);
    _CHECK_ALIGNMENT(Vec2);
    _CHECK_ALIGNMENT(Vec3);
    _CHECK_ALIGNMENT(Mat3);
    _CHECK_ALIGNMENT(Mat4);

    template<typename T>
    static void destructor(void* ptr) {
        reinterpret_cast<T*>(ptr)->~T();

        // FIXME: Is this the best place to do this? Should it
        // be at frame end?
        MaterialValuePool::get().clean_pointers();
    }

    // FIXME: Eugh, singleton. This should probably at least
    // be a property of the application.
    static MaterialValuePool& get() {
        static MaterialValuePool pool;
        return pool;
    }

    MaterialValuePool() {
        allocator_.set_realloc_callback(&MaterialValuePool::realloc_callback,
                                        this);
    }

    ~MaterialValuePool() {
        for(auto& ptr: pointers_) {
            if(ptr) {
                *(ptr.data_) = nullptr;
            }
        }
    }

    static void realloc_callback(uint8_t* old_data, uint8_t* new_data,
                                 void* user_data) {
        auto self = reinterpret_cast<MaterialValuePool*>(user_data);

        for(auto& pointer: self->pointers_) {
            *pointer.data_ = new_data + (*pointer.data_ - old_data);
        }
    }

    template<typename T>
    MaterialPropertyValuePointer get_or_create_value(const T& value) {
        for(auto& pointer: pointers_) {
            if(_impl::material_property_lookup<T>::type == pointer.type() &&
               *pointer.get<T>() == value) {
                return pointer;
            }
        }

        // Create a new value
        // 1. Allocate space for the value
        // 2. Create a new pointer and add it to the list
        // 3. return a copy

        auto data = std::make_shared<uint8_t*>(
            allocator_.allocate(alignment + sizeof(T)));
        BlockHeader* header = reinterpret_cast<BlockHeader*>(*data);
        header->refcount = 0;
        header->type = _impl::material_property_lookup<T>::type;
        header->destructor = &MaterialValuePool::destructor<T>;

        new(*data + alignment) T(value);

        MaterialPropertyValuePointer pointer(data);
        pointers_.push_back(pointer);
        return pointer;
    }

    void clean_pointers() {
        pointers_.erase(
            std::remove_if(pointers_.begin(), pointers_.end(),
                           [](const MaterialPropertyValuePointer& ptr) {
            return ptr.refcount() == 1;
        }),
            pointers_.end());
    }

private:
    std::vector<MaterialPropertyValuePointer> pointers_;
    DynamicAlignedAllocator<alignment> allocator_;
};

} // namespace smlt
