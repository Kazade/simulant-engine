#pragma once

#include "texture_loader.h"

namespace smlt {
namespace loaders {

/*
 * Ideally this wouldn't exist as SOIL does load DDS automatically, however, it doesn't
 * seem like there is any way to read the S3TC format back once it's loaded :(
 */

class DDSTextureLoader : public BaseTextureLoader {

};

}
}
