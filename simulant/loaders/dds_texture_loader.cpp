#include "dds_texture_loader.h"
#include "../macros.h"

namespace smlt {
namespace loaders {

TextureLoadResult DDSTextureLoader::do_load(std::shared_ptr<FileIfstream> stream) {
    _S_UNUSED(stream);
    throw std::logic_error("Not yet implemented");
}

}
}
