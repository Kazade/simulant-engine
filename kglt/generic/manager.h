#ifndef MANAGER_H
#define MANAGER_H

#include <map>
#include <stdexcept>
#include <sigc++/sigc++.h>

namespace kglt {
namespace generic {

class NoSuchObjectError : public std::logic_error {
public:
    NoSuchObjectError():
        std::logic_error("The manager does not contain an object with the specified ID") {}
};

class BaseManager {
public:
    virtual ~BaseManager() {}

protected:
    mutable boost::mutex manager_lock_;
};

template<typename T>
class IncrementalGetNextID {
public:
    T operator()() {
        static T counter = 0;
        return ++counter;
    }
};

template<typename ObjectType, typename ObjectIDType, typename NewIDGenerator=IncrementalGetNextID<ObjectIDType> >
class TemplatedManager : public virtual BaseManager {
public:
    ObjectIDType manager_new() {
        ObjectIDType id = 0;
        {
            boost::mutex::scoped_lock lock(manager_lock_);
            id = NewIDGenerator()();
            objects_.insert(std::make_pair(id, typename ObjectType::ptr(new ObjectType)));
        }

        signal_post_create_(*objects_[id], id);

        return id;
    }

    void manager_delete(ObjectIDType id) {
        if(manager_contains(id)) {
            ObjectType& obj = *objects_[id];
            signal_pre_delete_(obj, id); //Must happen outside the lock

            boost::mutex::scoped_lock lock(manager_lock_);
            if(manager_contains(id)) {
                objects_.erase(id);
            }
        }
    }

    ObjectType& manager_get(ObjectIDType id) {
        boost::mutex::scoped_lock lock(manager_lock_);

        if(!container::contains(objects_, id)) {
            throw NoSuchObjectError();
        }

        return *objects_[id];
    }

    const ObjectType& manager_get(ObjectIDType id) const {
        boost::mutex::scoped_lock lock(manager_lock_);
        if(!container::contains(objects_, id)) {
            throw NoSuchObjectError();
        }
        typename std::map<ObjectIDType, typename ObjectType::ptr>::const_iterator it = objects_.find(id);
        return *(it->second);
    }

    bool manager_contains(ObjectIDType id) {
        return objects_.find(id) != objects_.end();
    }

    sigc::signal<void, ObjectType&, ObjectIDType>& signal_post_create() { return signal_post_create_; }
    sigc::signal<void, ObjectType&, ObjectIDType>& signal_pre_delete() { return signal_pre_delete_; }

private:
    std::map<ObjectIDType, typename ObjectType::ptr> objects_;
    sigc::signal<void, ObjectType&, ObjectIDType> signal_post_create_;
    sigc::signal<void, ObjectType&, ObjectIDType> signal_pre_delete_;

protected:
    ObjectIDType _get_object_id_from_ptr(ObjectType* ptr) {
        for(std::pair<ObjectIDType, typename ObjectType::ptr> pair: objects_) {
            if(pair.second.get() == ptr) {
                return pair.first;
            }
        }

        return 0;
    }
};

}
}
#endif // MANAGER_H
