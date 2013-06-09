#ifndef RESOURCE_H
#define RESOURCE_H

#include <cassert>
#include <mutex>

namespace kglt {

class ResourceManager;

class Resource {
public:
    Resource(ResourceManager* manager):
        manager_(manager) {
        created_ = std::chrono::system_clock::now();
    }

    virtual ~Resource() {}

    ResourceManager& resource_manager() { assert(manager_); return *manager_; }

    std::recursive_mutex& mutex() { return mutex_; }

    int age() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
                    created_ - std::chrono::system_clock::now()
               ).count();
    }

private:
    ResourceManager* manager_;


    std::chrono::time_point<std::chrono::system_clock> created_;
    std::recursive_mutex mutex_;
};

}
#endif // RESOURCE_H
