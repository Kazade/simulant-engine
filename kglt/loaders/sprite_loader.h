#ifndef SPRITE_LOADER_H_INCLUDED
#define SPRITE_LOADER_H_INCLUDED

#include "kglt/loader.h"

namespace kglt {
namespace loaders {

class SpriteLoader : public Loader {
public:
    SpriteLoader(const std::string& filename):
        Loader(filename) {}

    void into(Loadable& resource, const kglt::option_list::OptionList& options);
    void into(Loadable& resource);
};

class SpriteLoaderType : public LoaderType {
public:
    std::string name() { return "sprite_loader"; }
    bool supports(const std::string& filename) const {
        return filename.find(".tga") != std::string::npos ||
                filename.find(".png") != std::string::npos;
    }

    bool has_hint(const std::string& type_hint) const {
        return type_hint == "LOADER_HINT_SPRITE";
    }

    Loader::ptr loader_for(const std::string& filename) const {
        return Loader::ptr(new SpriteLoader(filename));
    }
    
    bool requires_hint() const { return true; }
};

}
}

#endif // SPRITE_LOADER_H_INCLUDED
