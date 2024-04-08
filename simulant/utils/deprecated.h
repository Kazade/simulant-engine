#pragma once

#if __cplusplus==201402L
#define S_DEPRECATED [[deprecated]]
#else
    #ifdef __GNUC__
#define S_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define S_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define S_DEPRECATED
#endif
#endif
