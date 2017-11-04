#pragma once

/*
 * VQ compressed image loader. On Dreamcast this will store the texture with the correct compressed format
 * on other platforms we decompress in software (useful for testing DC compatible stuff without hardware)
 */

#include "texture_loader.h"

namespace smlt {
namespace loaders {

class KMGTextureLoader : public BaseTextureLoader {

};

}
}

