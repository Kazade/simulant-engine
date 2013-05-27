#include "resource.h"
#include "resource_manager.h"

namespace kglt {

void Resource::move_to_resource_manager(ResourceManager& destination) {
    if(&resource_manager() == &destination) {
        return;
    }

    destination.inherit(this);
}

}
