#ifndef RML_LOADER_H
#define RML_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class RMLLoader : public Loader {
public:
    RMLLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class RMLLoaderType : public LoaderType {
public:
    unicode name() { return "rml"; }
    bool supports(const unicode& filename) const {
        return filename.lower().contains(".rml");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new RMLLoader(filename, data));
    }
};

}
}

#endif // RML_LOADER_H
