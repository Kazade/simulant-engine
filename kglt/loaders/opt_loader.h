#ifndef OPT_LOADER_H
#define OPT_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class OPTLoader : public Loader {
public:
    OPTLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);

};

class OPTLoaderType : public LoaderType {
public:
    std::string name() { return "opt_loader"; }
    bool supports(const std::string& filename) const {
        //FIXME: check magic
        return filename.find(".opt") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new OPTLoader(filename));
    }
};


}
}

#endif // OPT_LOADER_H
