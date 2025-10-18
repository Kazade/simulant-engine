#include "dds_texture_loader.h"
#include "../macros.h"

namespace smlt {
namespace loaders {

bool DDSTextureLoader::do_load(std::shared_ptr<FileIfstream> stream,
                               Texture* tex) {
    _S_UNUSED(stream);
    _S_UNUSED(tex);
    return false;
}
}
}
