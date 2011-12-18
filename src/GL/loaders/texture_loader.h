#ifndef KGLT_TGA_LOADER_H
#define KGLT_TGA_LOADER_H

#include "../loader.h"

namespace GL {
namespace loaders {

class TextureLoader : public Loader {
public:
    TextureLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);
};

class TextureLoaderType : public LoaderType {
public:
    std::string name() { return "texture_loader"; }
    bool supports(const std::string& filename) const {
        return filename.find(".tga") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new TextureLoader(filename));
    }
};

}
}

#endif
