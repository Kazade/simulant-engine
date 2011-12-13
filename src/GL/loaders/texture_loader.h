#ifndef KGLT_TGA_LOADER_H
#define KGLT_TGA_LOADER_H

#include "../loader.h"

namespace GL {
namespace loaders {

class TextureLoader : public Loader {
public:
    void load_into(Resource& resource, const std::string& filename);
};

}
}

#endif
