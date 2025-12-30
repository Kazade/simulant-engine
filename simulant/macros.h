#pragma once

#define _S_UNUSED(x) (void)(x)

#if !defined(_MSC_VER)
#define _S_FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
#define _S_FORCE_INLINE static __forceinline
#endif

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

// https://stackoverflow.com/a/11172679

/* expands to the first argument */
#define _S_FIRST(...) _S_FIRST_HELPER(__VA_ARGS__, throwaway)
#define _S_FIRST_HELPER(first, ...) first

/*
 * if there's only one argument, expands to nothing.  if there is more
 * than one argument, expands to a comma followed by everything but
 * the first argument.  only supports up to 9 arguments but can be
 * trivially expanded.
 */
#define _S_REST(...) _S_REST_HELPER(_S_NUM(__VA_ARGS__), __VA_ARGS__)
#define _S_REST_HELPER(qty, ...) _S_REST_HELPER2(qty, __VA_ARGS__)
#define _S_REST_HELPER2(qty, ...) _S_REST_HELPER_##qty(__VA_ARGS__)
#define _S_REST_HELPER_ONE(first)
#define _S_REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define _S_NUM(...)                                                            \
    _S_SELECT_10TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE,    \
                   TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define _S_SELECT_10TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10
