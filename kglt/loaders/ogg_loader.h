#ifndef OGG_LOADER_H
#define OGG_LOADER_H

#include <map>
#include <vector>
#include "../types.h"
#include "../loader.h"

namespace kglt {
namespace loaders {

class OGGLoader : public Loader {
public:
    OGGLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions &options=LoaderOptions());

};

class OGGLoaderType : public LoaderType {
public:
    unicode name() { return "ogg"; }
    bool supports(const unicode& filename) const {
        //FIXME: check magic
        return filename.lower().contains(".ogg");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new OGGLoader(filename, data));
    }
};


}
}

#endif // OGG_LOADER_H
