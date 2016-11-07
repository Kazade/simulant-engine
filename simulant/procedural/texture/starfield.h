#ifndef STARFIELD_H
#define STARFIELD_H

#include <cstdint>

namespace smlt {

class Texture;

namespace procedural {
namespace texture {
    void starfield(smlt::TexturePtr texture_ptr, uint32_t width=1024, uint32_t height=1024);
}
}
}


#endif // STARFIELD_H
