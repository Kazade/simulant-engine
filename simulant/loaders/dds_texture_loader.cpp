#include "dds_texture_loader.h"
#include "../macros.h"

namespace smlt {
namespace loaders {

TextureLoadResult DDSTextureLoader::do_load(const std::vector<uint8_t> &buffer) {
    _S_UNUSED(buffer);
    throw std::logic_error("Not yet implemented");
}

}
}
