SET(CMAKE_SYSTEM_NAME Windows)

SET(MINGW_PREFIX x86_64-w64-mingw32)

SET(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
SET(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX} /usr/${MINGW_PREFIX}/sys-root/mingw /usr/${MINGW_PREFIX}/sys-root/mingw/include)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
