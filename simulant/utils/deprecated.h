#pragma once

#if __cplusplus==201402L
#define DEPRECATED [[deprecated]]
#else
    #ifdef __GNUC__
    #define DEPRECATED __attribute__((deprecated))
    #elif defined(_MSC_VER)
    #define DEPRECATED __declspec(deprecated)
    #else
    #pragma message("WARNING: You need to implement DEPRECATED for this compiler")
    #define DEPRECATED
    #endif
#endif
