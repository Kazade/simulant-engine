#ifndef RESOURCE_H
#define RESOURCE_H

#include <cassert>

namespace kglt {

class ResourceManager;

class Resource {
public:
    Resource(ResourceManager* manager):
        manager_(manager) {}

    virtual ~Resource() {}

    ResourceManager& resource_manager() { assert(manager_); return *manager_; }

private:
    ResourceManager* manager_;
};

}
#endif // RESOURCE_H
