#pragma once

/* SpritesheetAttrs has no user-declared constructor, so cgen.py can't
 * safely assume one is synthesizable (see ports/c/README.md) and skips
 * smlt_spritesheet_attrs_create() -- same situation as AppConfig in
 * application_ext.h. This is the hand-verified equivalent, giving the
 * engine's normal defaults (no margin/spacing/padding). */

#include "simulant/c/types.h"

#ifdef __cplusplus
extern "C" {
#endif

smlt_spritesheet_attrs_t* smlt_spritesheet_attrs_create_default(void);

#ifdef __cplusplus
}
#endif
