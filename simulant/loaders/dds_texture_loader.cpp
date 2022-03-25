#include "dds_texture_loader.h"
#include "../macros.h"

namespace smlt {
namespace loaders {

optional<TextureLoadResult> DDSTextureLoader::do_load(std::shared_ptr<FileIfstream> stream) {
    _S_UNUSED(stream);
    S_ERROR("Not yet implemented");
    return optional<TextureLoadResult>();
}

}
}
