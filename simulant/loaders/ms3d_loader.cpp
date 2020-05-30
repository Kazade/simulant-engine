#include "ms3d_loader.h"

namespace smlt {
namespace loaders {

MS3DLoader::MS3DLoader(const unicode& filename, std::shared_ptr<std::istream> data):
    Loader(filename, data) {}

void MS3DLoader::into(Loadable& resource, const LoaderOptions& options) {

}

}
}
