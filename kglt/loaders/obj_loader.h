#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class OBJLoader : public Loader {
public:
    OBJLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class OBJLoaderType : public LoaderType {
public:
    OBJLoaderType() {

    }

    ~OBJLoaderType() {}

    unicode name() { return "obj_loader"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".obj");
    }

    Loader::ptr loader_for(const unicode& filename) const {
        return Loader::ptr(new OBJLoader(filename));
    }
};

}
}

#endif // OBJ_LOADER_H
