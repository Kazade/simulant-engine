SET(CMAKE_SYSTEM_NAME Windows)

SET(MINGW_PREFIX x86_64-w64-mingw32)

SET(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
SET(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX} /usr/${MINGW_PREFIX}/lib /usr/${MINGW_PREFIX}/include)


# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


#list(APPEND CMAKE_PREFIX_PATH "/usr/${MINGW_PREFIX}/")
#SET(SDL2_LIBRARY "/usr/x86_64-w64-mingw32/lib/libSDL2.dll.a")
SET(SDL2_INCLUDE_DIR "/usr/x86_64-w64-mingw32/include/SDL2")
#find_library(ZLIB_LIBRARY z HINTS "/usr/x86_64-w64-mingw32/lib")
#SET(ZLIB_INCLUDE_DIR "/usr/x86_64-w64-mingw32/include")

add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DWIN32 -D_WIN32 -D_WIN64 -D__WIN32__)

add_definitions(-D__USE_MINGW_ANSI_STDIO=1)

set(MINGW 1)
#set(WIN32 1)
#set(WIN64 1)