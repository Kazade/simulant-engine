#pragma once

#include <vector>

#include "../../../generic/allocators/dynamic_aligned_allocator.h"
#include "../property_value.h"

namespace smlt {

static constexpr int alignment = 4;

struct BlockHeader {
    MaterialPropertyType type;
    uint16_t refcount = 0;
};

/* We need to be able to defrag the material value pool, so we return pointers
   which are ref-counted to tell if values are still used and maintain our own
   copies in the pool so we can update pointers if memory shifts around */
class MaterialPropertyValuePointer {
    std::shared_ptr<uint8_t*> data_;

public:
    MaterialPropertyValuePointer() = default;
    MaterialPropertyValuePointer(const MaterialPropertyValuePointer& other) {
        data_ = other.data_;

        if(data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            ++header->refcount;
        }
    }

    MaterialPropertyValuePointer&
        operator=(const MaterialPropertyValuePointer& other) {
        if(this == &other) {
            return *this;
        }

        data_ = other.data_;
        if(data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            ++header->refcount;
        }
        return *this;
    }

    ~MaterialPropertyValuePointer() {
        if(data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            ++header->refcount;
        }
    }

    template<typename T>
    T& get() {
        return *reinterpret_cast<T*>((*data_) + alignment);
    }

    template<typename T>
    const T& get() const {
        return *reinterpret_cast<const T*>((*data_) + alignment);
    }

    // Warning: don't call this on a null pointer
    MaterialPropertyType type() const {
        BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
        return header->type;
    }

    void reset(std::shared_ptr<uint8_t*> data) {
        if(data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            --header->refcount;
        }

        fprintf(stderr, "New pointer at 0x%x\n", data);
        data_ = data;

        if(data_) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            header->refcount++;
        }
    }
};

class MaterialValuePool {
public:
    static_assert(sizeof(BlockHeader) <= alignment, "Alignment is too small");
    static_assert((alignof(float) % alignment) == 0,
                  "Invalid alignment for float");
    static_assert((alignof(Vec2) % alignment) == 0,
                  "Invalid alignment for Vec2");
    static_assert((alignof(Vec3) % alignment) == 0,
                  "Invalid alignment for Vec3");
    static_assert((alignof(Mat3) % alignment) == 0,
                  "Invalid alignment for Mat3");
    static_assert((alignof(Mat4) % alignment) == 0,
                  "Invalid alignment for Mat4");

    MaterialValuePool() {
        allocator_.set_realloc_callback(&MaterialValuePool::realloc_callback,
                                        this);
    }

    ~MaterialValuePool() {
        for(auto& ptr: pointers_) {
            ptr.data_ = nullptr;
        }
    }

    static void realloc_callback(uint8_t* old_data, uint8_t* new_data,
                                 void* user_data) {
        auto self = reinterpret_cast<MaterialValuePool*>(user_data);

        for(auto& pointer: self->pointers_) {
            fprintf(stderr, "Updating 0x%x to 0x%x\n", *pointer.data_,
                    new_data + (*pointer.data_ - old_data));
            *pointer.data_ = new_data + (*pointer.data_ - old_data);
        }
    }

    template<typename T>
    MaterialPropertyValuePointer get_or_create_value(const T& value) {
        for(auto& pointer: pointers_) {
            if(_impl::material_property_lookup<T>::type == pointer.type() &&
               pointer.get<T>() == value) {
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

        new(*data + alignment) T(value);

        MaterialPropertyValuePointer pointer;
        pointer.reset(data);

        pointers_.push_back(pointer);
        return pointer;
    }

private:
    std::vector<MaterialPropertyValuePointer> pointers_;
    DynamicAlignedAllocator<alignment> allocator_;
};

} // namespace smlt
