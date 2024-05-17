#pragma once

#include <memory>

#include "../../assets/material.h"

#define _PVR_MAX_TEXTURE_UNITS 1

namespace smlt {

struct PVRRenderGroupImpl {
    int texture_id[_PVR_MAX_TEXTURE_UNITS] = {0};
};
}

