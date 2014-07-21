#ifndef PARTICLE_SCRIPT_H
#define PARTICLE_SCRIPT_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class KGLPLoader : public Loader {
public:
    KGLPLoader(const unicode& filename):
        Loader(filename) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class KGLPLoaderType : public LoaderType {
public:
    KGLPLoaderType() {

    }

    ~KGLPLoaderType() {}

    unicode name() { return "kglp_loader"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".kglp");
    }

    Loader::ptr loader_for(const unicode& filename) const {
        return Loader::ptr(new KGLPLoader(filename));
    }
};

}
}

#endif // PARTICLE_SCRIPT_H
