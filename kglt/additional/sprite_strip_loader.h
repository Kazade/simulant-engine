#ifndef SPRITE_STRIP_LOADER_H
#define SPRITE_STRIP_LOADER_H

#include <cstdint>
#include <vector>
#include <string>
#include "../generic/creator.h"
#include "../types.h"

namespace kglt {

class Scene;

namespace additional {

class SpriteStripLoader :
        public generic::Creator<SpriteStripLoader> {
public:
    SpriteStripLoader(Scene& scene, const std::string& filename, uint32_t frame_width);
    std::vector<TextureID> load_frames();

private:
    Scene& scene_;
    std::string filename_;
    uint32_t frame_width_;
};

}
}

#endif // SPRITE_STRIP_LOADER_H
