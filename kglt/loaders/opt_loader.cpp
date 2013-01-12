#include "../mesh.h"

#include "opt_loader.h"

namespace kglt {
namespace loaders {

void OPTLoader::into(Loadable& resource) {
    Loadable* res_ptr = &resource;
    Mesh* scene = dynamic_cast<Mesh*>(res_ptr);
    assert(scene && "You passed a Resource that is not a mesh to the OPT loader");

    std::ifstream file(filename_.c_str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the OPT file: " + filename_);
    }



}

}
}
