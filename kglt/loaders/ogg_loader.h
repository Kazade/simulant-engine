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
    OGGLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);

};

class OGGLoaderType : public LoaderType {
public:
    std::string name() { return "ogg_loader"; }
    bool supports(const std::string& filename) const {
        //FIXME: check magic
        return filename.find(".ogg") != std::string::npos ||
               filename.find(".OGG") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new OGGLoader(filename));
    }
};


}
}

#endif // OGG_LOADER_H
