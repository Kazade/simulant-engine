#pragma once

#define _S_UNUSED(x) (void)(x)
#ifndef __WIN32__
#define _S_FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
#define _S_SFORCE_INLINE static __forceinline
#endif
#define _S_ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))
