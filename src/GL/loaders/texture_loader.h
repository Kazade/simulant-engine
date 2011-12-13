#ifndef KGLT_TGA_LOADER_H
#define KGLT_TGA_LOADER_H

#include "../loader.h"

namespace GL {
namespace loaders {

class TextureLoader : public Loader {
public:
    void load_into(Resource& resource, const std::string& filename);
};

class TextureLoaderType : public LoaderType {
public:
    std::string name() { return "texture_loader"; }
    bool supports(const std::string& filename) {
        return str::ends_with(filename, ".tga");
    }
    
    Loader loader_for(const std::string& filename) const {
        return TextureLoader(filename);
    }
};

}
}

#endif
