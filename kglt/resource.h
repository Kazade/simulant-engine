#ifndef RESOURCE_H
#define RESOURCE_H

#include <cassert>
#include <mutex>

#include "generic/protected_ptr.h"
#include "generic/data_carrier.h"

namespace kglt {

class ResourceManager;

class Resource :
    public virtual Protectable,
    public virtual generic::DataCarrier {

public:
    Resource(ResourceManager* manager):
        manager_(manager) {
        created_ = std::chrono::system_clock::now();
    }

    virtual ~Resource() {}

    ResourceManager& resource_manager() { assert(manager_); return *manager_; }
    const ResourceManager& resource_manager() const { assert(manager_); return *manager_; }

    int age() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
                    created_ - std::chrono::system_clock::now()
               ).count();
    }

private:
    ResourceManager* manager_;

    std::chrono::time_point<std::chrono::system_clock> created_;
};

}
#endif // RESOURCE_H
