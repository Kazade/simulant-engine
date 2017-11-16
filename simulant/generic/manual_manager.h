#pragma once

#include <mutex>
#include <vector>
#include <unordered_set>
#include <numeric>
#include <cmath>
#include <cstring>

#include "../deps/kazsignal/kazsignal.h"
#include "../deps/kazlog/kazlog.h"

namespace smlt {
namespace generic {

template<typename ObjectType, typename ObjectIDType>
class ManualManager {
protected:
    mutable std::recursive_mutex manager_lock_;

    template<typename Q=ObjectIDType>
    typename ObjectIDType::resource_pointer_type getter(const ObjectIDType* id) {
        return get(*id);
    }

public:
    ManualManager():
        objects_(this) {}

    template<typename... Args>
    ObjectIDType make(Args&&... args) {
        return make_as<ObjectType>(std::forward<Args>(args)...);
    }

    template<typename... Args>
    ObjectIDType make(ObjectIDType id, Args&&... args) {
        return make_as<ObjectType>(id, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    ObjectIDType make_as(Args&&... args) {

        // Make the new object, but dont init until outside the lock
        // this prevents deadlocks being caused if a stage is created in another
        // thread and needs the idle task handler to run

        // Use the create method, but then copy the object into the container
        ObjectType* inserted = nullptr;
        {
            std::lock_guard<std::recursive_mutex> lock(manager_lock_);
            inserted = &objects_.create(

                std::forward<Args>(args)...
            );

        }

        inserted->init();

        signal_post_create_(*inserted, inserted->id());

        return inserted->id();
    }

    void destroy(ObjectIDType id) {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);
        if(contains(id)) {
            // Call destructor
            auto obj = get(id);

            signal_pre_delete_(*obj, id);

            obj->cleanup();
            objects_.erase(id.value() - 1);
        }
    }

    ObjectType* get(ObjectIDType id) const {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto slot = id.value() - 1;
        if(!objects_.is_occupied_slot(slot)) {
            L_WARN(_F(
                "Unable to find object of type: {0} with ID {1}").format(
                    typeid(ObjectType).name(),
                    id
                )
            );
            return nullptr;
        }

        return &objects_[slot];
    }

    bool contains(ObjectIDType id) const {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);
        auto slot = id.value() - 1;
        return objects_.is_occupied_slot(slot);
    }

    std::size_t count() const {
        return objects_.size();
    }

    void each(std::function<void (uint32_t, ObjectType*)> func) const {
        uint32_t j = 0;

        std::lock_guard<std::recursive_mutex> lock(manager_lock_);
        for(std::size_t i = 0; i < objects_.capacity(); ++i) {
            if(!objects_.is_occupied_slot(i)) {
                // Ignore empty slots
                continue;
            }

            auto thing = objects_[i];
            func(j++, thing);
        }
    }

    void clear() {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        for(std::size_t i = 0; i < objects_.capacity(); ++i) {
            if(!objects_.is_occupied_slot(i)) {
                // Ignore empty slots
                continue;
            }

            auto thing = &objects_[i];
            destroy(thing->id());
        }

        objects_.clear();
    }

private:
    class Wrapper {
    private:
        const static std::size_t BUFFER_SIZE_IN_ELEMENTS = 64;

        std::unordered_set<std::size_t> free_slots_;

        typedef std::vector<uint8_t> Buffer;
        std::vector<Buffer> buffers_;

        std::size_t count_ = 0;

        void new_buffer() {
            // Create a new buffer on the stack
            buffers_.push_back(Buffer());

            // Resize that buffer to the right size
            buffers_.back().resize(BUFFER_SIZE_IN_ELEMENTS * sizeof(ObjectType));

            // Generate a new set of slot IDs and add them to the free slots
            std::vector<std::size_t> new_slots(BUFFER_SIZE_IN_ELEMENTS);
            std::iota(new_slots.begin(), new_slots.end(), (buffers_.size() - 1) * BUFFER_SIZE_IN_ELEMENTS);
            free_slots_.insert(new_slots.rbegin(), new_slots.rend());
        }

        void resize(std::size_t elements) {
            auto required_buffers = std::size_t(std::ceil(float(elements + 1) / float(BUFFER_SIZE_IN_ELEMENTS)));

            if(buffers_.size() < required_buffers) {
                while(buffers_.size() < required_buffers) {
                    new_buffer();
                }
            } else {
                buffers_.resize(required_buffers);
                auto max_slot = buffers_.size() * BUFFER_SIZE_IN_ELEMENTS;

                // If we shrank the capacity, we need to remove any free slots which are
                // no longer valid
                for(auto it = free_slots_.begin(); it != free_slots_.end();) {
                    auto slot = (*it);
                    if(slot > max_slot) {
                        it = free_slots_.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        ManualManager<ObjectType, ObjectIDType>* parent_;

    public:
        Wrapper(ManualManager<ObjectType, ObjectIDType>* parent):
            parent_(parent) {

            new_buffer();
        }

        std::size_t capacity() const {
            return BUFFER_SIZE_IN_ELEMENTS * buffers_.size();
        }

        bool is_occupied_slot(const std::size_t& i) const {
            if(free_slots_.count(i)) {
                return false;
            }

            return i < capacity();
        }

        ObjectType& operator[](std::size_t i) const {
            auto buffer_id = i / BUFFER_SIZE_IN_ELEMENTS;
            auto buffer_idx = i % BUFFER_SIZE_IN_ELEMENTS;

            auto& buffer = buffers_[buffer_id];

            assert(buffer_idx * sizeof(ObjectType) < buffer.size());

            const uint8_t* ptr = &buffer.at(sizeof(ObjectType) * buffer_idx);
            return *((ObjectType*) ptr);
        }

        template<typename... Args>
        ObjectType& create(Args&&... args) {
            std::size_t slot = 0;

            if(!free_slots_.empty()) {
                slot = *free_slots_.begin();
            } else {
                // If there are no free slots, then we must have filled the
                // available buffers, so we ask for the next slot
                // which will force a resize
                slot = count_;
                if(slot >= capacity()) {
                    resize(slot);
                }
            }

            // Make sure that the slot we need isn't marked as free
            free_slots_.erase(slot);

            void* dest = &(*this)[slot];

            // FIXME: it would be nice if ObjectIDType wasn't instantiated here, but I can't find a clean
            // way of getting the slot ID otherwise
            auto id = slot + 1;
            auto inserted = new (dest) ObjectType(
                ObjectIDType(id, [this](const ObjectIDType* object_id) -> typename ObjectIDType::resource_pointer_type {
                    return parent_->getter(object_id);
                }), std::forward<Args>(args)...
            );
            ++count_;

            assert(inserted);

            return *inserted;
        }

        std::size_t size() const {
            return count_;
        }

        void erase(std::size_t i) {
            assert(free_slots_.count(i) == 0);
            ObjectType* obj = &(*this)[i];
            obj->~ObjectType();
            std::memset(obj, 0, sizeof(ObjectType));
            free_slots_.insert(i);
            --count_;
        }

        void clear() {
            for(std::size_t i = 0; i < capacity(); ++i) {
                if(free_slots_.count(i)) continue;
                erase(i);
            }
            assert(count_ == 0);

            buffers_.clear();
            free_slots_.clear();
        }
    };

    Wrapper objects_;

    sig::signal<void (ObjectType&, ObjectIDType)> signal_post_create_;
    sig::signal<void (ObjectType&, ObjectIDType)> signal_pre_delete_;
};


}
}
