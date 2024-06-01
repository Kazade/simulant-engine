#pragma once

#define _S_UNUSED(x) (void)(x)
#define _S_FORCE_INLINE __attribute__((always_inline)) static __inline__
#define _S_ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

#if defined(_MSC_VER)
#define _S_RESTRICT __restrict
#else
#define _S_RESTRICT __restrict__
#endif

// Based on: https://stackoverflow.com/questions/7895869/cross-platform-alignx-macro
#if defined(__GNUC__) || defined(__clang__)
#define _S_ALIGN(x) __attribute__((aligned(x)))
#elif defined(_MSC_VER)
#define _S_ALIGN(x) alignas(x)
#else
#error "Unable to define _S_ALIGN due to unknown compiler"
#endif
