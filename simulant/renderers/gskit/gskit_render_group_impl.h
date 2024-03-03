#pragma once

#include <memory>

#include "../../assets/material.h"

#define _GS_KIT_MAX_TEXTURE_UNITS 1

namespace smlt {

struct GSKitRenderGroupImpl {
    int texture_id[_GS_KIT_MAX_TEXTURE_UNITS] = {0};
};


}
