#pragma once

#define _S_UNUSED(x) (void)(x)
#define _S_FORCE_INLINE __attribute__((always_inline)) static __inline__
#define _S_ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))
