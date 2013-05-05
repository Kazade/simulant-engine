#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class OBJLoader : public Loader {
public:
    OBJLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);
    void into(Loadable& resource, const kglt::option_list::OptionList& options);
};

class OBJLoaderType : public LoaderType {
public:
    OBJLoaderType() {

    }

    ~OBJLoaderType() {}

    std::string name() { return "obj_loader"; }
    bool supports(const std::string& filename) const override {
        return filename.find(".obj") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new OBJLoader(filename));
    }
};

}
}

#endif // OBJ_LOADER_H
