#pragma once

#include <memory>

#include "../../assets/material.h"

#define _PSP_MAX_TEXTURE_UNITS 1

namespace smlt {

struct PSPRenderGroupImpl {
    int texture_id[_PSP_MAX_TEXTURE_UNITS] = {0};
};


}

