#pragma once

#define _S_UNUSED(x) (void)(x)

#if !defined(_MSC_VER)
#define _S_FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
#define _S_FORCE_INLINE static __forceinline
#endif

#define _S_ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))
