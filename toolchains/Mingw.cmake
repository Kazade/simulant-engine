set(CMAKE_SYSTEM_NAME Windows)

set(MINGW 1)
set(MINGW_PREFIX x86_64-w64-mingw32)

set(CMAKE_CROSSCOMPILING 1)

set(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

if(EXISTS "/usr/${MINGW_PREFIX}/sys-root")
    set(CMAKE_SYSROOT /usr/${MINGW_PREFIX}/sys-root)
else()
    set(CMAKE_SYSROOT /usr/${MINGW_PREFIX})
endif()

set(CMAKE_PREFIX_PATH /usr/${MINGW_PREFIX} /usr/${MINGW_PREFIX}/sys-root/mingw)

# here is the target environment located
set(
    CMAKE_FIND_ROOT_PATH
    /usr/${MINGW_PREFIX}
    /usr/${MINGW_PREFIX}/include
    /usr/${MINGW_PREFIX}/lib
    /usr/${MINGW_PREFIX}/sys-root
    /usr/${MINGW_PREFIX}/sys-root/mingw/include
    /usr/${MINGW_PREFIX}/sys-root/mingw/lib
)


# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DWIN32 -D_WIN32 -D_WIN64 -D__WIN32__)

add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
