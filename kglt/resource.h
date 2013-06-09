#ifndef RESOURCE_H
#define RESOURCE_H

#include <cassert>
#include <mutex>

namespace kglt {

class ResourceManager;

class Resource {
public:
    Resource(ResourceManager* manager):
        manager_(manager) {}

    virtual ~Resource() {}

    ResourceManager& resource_manager() { assert(manager_); return *manager_; }

    std::recursive_mutex& mutex() { return mutex_; }

private:
    ResourceManager* manager_;

    std::recursive_mutex mutex_;
};

}
#endif // RESOURCE_H
