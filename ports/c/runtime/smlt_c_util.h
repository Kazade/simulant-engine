#pragma once

/* Small helpers shared by the generated C wrapper sources. Not part of the
 * public API -- only included by files under ports/c/generated/src. */

#ifdef __cplusplus
extern "C" {
#endif

char* smlt_c_strdup(const char* s);

#ifdef __cplusplus
}
#endif
