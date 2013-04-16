#ifndef RML_LOADER_H
#define RML_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class RMLLoader : public Loader {
public:
    RMLLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);
};

class RMLLoaderType : public LoaderType {
public:
    std::string name() { return "texture_loader"; }
    bool supports(const std::string& filename) const {
        return filename.find(".rml") != std::string::npos ||
               filename.find(".RML") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new RMLLoader(filename));
    }
};

}
}

#endif // RML_LOADER_H
