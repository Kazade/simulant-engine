#ifndef KGLT_TGA_LOADER_H
#define KGLT_TGA_LOADER_H

#include "../loader.h"

namespace kglt {
namespace loaders {

class TextureLoader : public Loader {
public:
    TextureLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource);
    void into(Loadable& resource, const kglt::option_list::OptionList& options);
};

class TextureLoaderType : public LoaderType {
public:
    TextureLoaderType() {

    }

    ~TextureLoaderType() {}

    std::string name() { return "texture_loader"; }
    bool supports(const std::string& filename) const override {
        return filename.find(".tga") != std::string::npos ||
               filename.find(".png") != std::string::npos;
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new TextureLoader(filename));
    }
};

}
}

#endif
