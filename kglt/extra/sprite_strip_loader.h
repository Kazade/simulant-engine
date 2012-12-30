#ifndef SPRITE_STRIP_LOADER_H
#define SPRITE_STRIP_LOADER_H

#include <cstdint>
#include <vector>
#include <string>
#include "../generic/creator.h"
#include "../types.h"
#include "../resource_manager.h"

namespace kglt {

class Scene;

namespace extra {

class SpriteStripLoader :
        public generic::Creator<SpriteStripLoader> {
public:
    SpriteStripLoader(ResourceManager& rm, const std::string& filename, uint32_t frame_width);
    std::vector<TextureID> load_frames();

private:
    ResourceManager& rm_;
    std::string filename_;
    uint32_t frame_width_;
};

}
}

#endif // SPRITE_STRIP_LOADER_H
