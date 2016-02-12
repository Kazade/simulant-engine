#ifndef PARTICLE_SCRIPT_H
#define PARTICLE_SCRIPT_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class KGLPLoader : public Loader {
public:
    KGLPLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class KGLPLoaderType : public LoaderType {
public:
    KGLPLoaderType() {

    }

    ~KGLPLoaderType() {}

    unicode name() { return "particle"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".kglp");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new KGLPLoader(filename, data));
    }
};

}
}

#endif // PARTICLE_SCRIPT_H
