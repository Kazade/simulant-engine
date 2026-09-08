#pragma once

/* InputAxis::set_positive_keyboard_key()/set_negative_keyboard_key() take
 * `const KeyboardCode&` in C++, so cgen.py maps them to a
 * `const smlt_keyboard_code_t*` C parameter (see the pointer-to-enum
 * branch in tools/cgen/cgen_lib/typemap.py). The Vala emitter doesn't
 * know to take the address of a plain enum value for that case, so the
 * generated .vapi binding for these two ends up calling the pointer
 * versions with a bare int -- these hand-written by-value equivalents
 * work around that from Vala (or any other by-value-preferring caller)
 * without touching the generator this late. */

#include "simulant/c/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void smlt_input_axis_set_positive_keyboard_key_value(smlt_input_axis_t* self, smlt_keyboard_code_t key);
void smlt_input_axis_set_negative_keyboard_key_value(smlt_input_axis_t* self, smlt_keyboard_code_t key);

#ifdef __cplusplus
}
#endif
