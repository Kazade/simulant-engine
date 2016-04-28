#pragma once

#ifdef KGLT_GL_VERSION_1X
#include "gl1x/batcher.h"
#include "gl1x/renderer.h"
#else
#include "gl2x/batcher.h"
#include "gl2x/generic_renderer.h"
#endif
