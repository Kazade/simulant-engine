#pragma once

#include <list>
#include <vector>

#include "../../../generic/allocators/dynamic_aligned_allocator.h"
#include "../property_value.h"

namespace smlt {

/* We have to use a 32 byte alignment, because
 * the destructor pointer takes up 8-bytes on 64 bit systems.
 * It would be nice not to be so wasteful, but in the grand scheme
 * of things we're saving a tonne of memory by using a flyweight pattern
 * anyway, so we'll still be saving space overall. */

// On 32 bit platforms we can reduce it a bit
static constexpr int alignment = sizeof(int*) == 4 ? 16 : 32;

class MaterialValuePool;

struct BlockHeader {
    MaterialPropertyType type;
    uint16_t refcount = 0;
    MaterialValuePool* pool = nullptr;
    void (*destructor)(void* ptr);
};

/* We need to be able to defrag the material value pool, so we return pointers
   which are ref-counted to tell if values are still used and maintain our own
   copies in the pool so we can update pointers if memory shifts around */
class MaterialPropertyValuePointer {
    std::weak_ptr<bool> pool_alive_;
    uint8_t** data_ = nullptr;

    friend class MaterialValuePool;

    MaterialPropertyValuePointer(const std::shared_ptr<bool>& alive_check,
                                 uint8_t** data) :
        pool_alive_(alive_check), data_(data) {

        increase_refcount();
    }

    void increase_refcount() {
        if(data_ && !pool_alive_.expired()) {
            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            ++header->refcount;
        }
    }

    void decrease_refcount() {
        if(data_ && !pool_alive_.expired()) {

            BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
            assert(header->refcount > 0);

            --header->refcount;

            if(header->refcount == 0) {
                header->destructor(*data_);
                data_ = nullptr;
                pool_alive_ = std::weak_ptr<bool>();
            }
        }
    }

public:
    MaterialPropertyValuePointer() = default;
    MaterialPropertyValuePointer(const MaterialPropertyValuePointer& other) :
        pool_alive_(other.pool_alive_), data_(other.data_) {

        increase_refcount();
    }

    MaterialPropertyValuePointer&
        operator=(const MaterialPropertyValuePointer& other) {
        if(this == &other) {
            return *this;
        }

        decrease_refcount();

        pool_alive_ = other.pool_alive_;
        data_ = other.data_;

        increase_refcount();

        return *this;
    }

    ~MaterialPropertyValuePointer() {
        decrease_refcount();
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
        assert(*this);

        BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
        return header->type;
    }

    std::size_t refcount() const {
        if(!data_) {
            return 0;
        }

        if(!pool_alive_.lock()) {
            return 0;
        }

        BlockHeader* header = reinterpret_cast<BlockHeader*>(*data_);
        return header->refcount;
    }

    void reset() {
        decrease_refcount();
        data_ = nullptr;
        pool_alive_ = std::weak_ptr<bool>();
    }

    operator bool() const {
        return data_ && !pool_alive_.expired();
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

    MaterialValuePool& operator=(const MaterialValuePool&) = delete;
    MaterialValuePool(const MaterialValuePool&) = delete;

    template<typename T>
    static void destructor(void* ptr) {
        BlockHeader* header = reinterpret_cast<BlockHeader*>(ptr);
        void* object = ((uint8_t*)ptr) + alignment;
        reinterpret_cast<T*>(object)->~T();
        header->pool->release(ptr);
    }

    MaterialValuePool() {
        allocator_.set_realloc_callback(&MaterialValuePool::realloc_callback,
                                        this);
    }

    void clear() {
        // Wipe the previous alive check, we're killing everything
        alive_check_ = std::make_shared<bool>(true);

        auto ptr = pointers_;

        while(ptr) {
            // Forcibly call the destructor
            auto header = reinterpret_cast<BlockHeader*>(ptr->data);
            // We're killing this, all pointers are now invalid
            header->refcount = 0;
            header->destructor(ptr->data);
            header->pool = nullptr;
            ptr = ptr->next;
        }

        auto it = pointers_;
        while(it) {
            auto next = it->next;
            delete it;
            it = next;
        }

        pointers_ = nullptr;
    }

    ~MaterialValuePool() {
        clear();
    }

    static void realloc_callback(uint8_t* old_data, uint8_t* new_data,
                                 void* user_data) {
        auto self = reinterpret_cast<MaterialValuePool*>(user_data);

        auto it = self->pointers_;
        while(it) {
            it->data = new_data + (it->data - old_data);
            it = it->next;
        }
    }

    template<typename T>
    MaterialPropertyValuePointer get_or_create_value(const T& value) {
        auto it = pointers_;
        while(it) {
            auto mvp = MaterialPropertyValuePointer(alive_check_, &it->data);
            if(_impl::material_property_lookup<T>::type == mvp.type() &&
               *mvp.get<T>() == value) {
                return mvp;
            }

            it = it->next;
        }

        // Create a new value
        // 1. Allocate space for the value
        // 2. Create a new pointer and add it to the list
        // 3. return a copy

        auto header_size = alignment;
        auto data = allocator_.allocate(header_size + sizeof(T));
        BlockHeader* header = reinterpret_cast<BlockHeader*>(data);
        header->refcount = 0;
        header->type = _impl::material_property_lookup<T>::type;
        header->pool = this;
        header->destructor = &MaterialValuePool::destructor<T>;

        new(data + header_size) T(value);

        auto entry = push_entry(data);

        MaterialPropertyValuePointer pointer(alive_check_, &entry->data);

        return pointer;
    }

    void release(void* ptr) {
#ifndef NDEBUG
        auto it = reinterpret_cast<BlockHeader*>(ptr);
        assert(it->refcount == 0);
#endif
        remove_entry((uint8_t*)ptr);

        // Deallocate the mem
        allocator_.deallocate((uint8_t*)ptr);
    }

private:
    struct PointerEntry {
        uint8_t* data = nullptr;
        PointerEntry* next = nullptr;
        PointerEntry* prev = nullptr;
    };

    PointerEntry* pointers_ = nullptr;

    PointerEntry* push_entry(uint8_t* data) {
        auto new_entry = new PointerEntry();
        new_entry->data = data;
        new_entry->next = pointers_;
        if(pointers_) {
            pointers_->prev = new_entry;
        }
        pointers_ = new_entry;
        return new_entry;
    }

    void remove_entry(uint8_t* data) {
        auto it = pointers_;
        while(it) {
            if(it->data == data) {
                if(it->prev) {
                    it->prev->next = it->next;
                } else {
                    pointers_ = it->next;
                }

                if(it->next) {
                    it->next->prev = it->prev;
                }

                delete it;
                return;
            }

            it = it->next;
        }
    }

    DynamicAlignedAllocator<alignment> allocator_;

    // This is used to populate weak_ptrs on the MaterialPropertyValuePointers
    // so that if the pool is destroyed, then those MPVPs get immediately
    // invalidated
    std::shared_ptr<bool> alive_check_ = std::make_shared<bool>(true);
};

} // namespace smlt
