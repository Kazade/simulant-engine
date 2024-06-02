# This module defines SIMULANT_FOUND, SIMULANT_LIBRARY AND SIMULANT_INCLUDE_DIR
#
# It prioritises architecture-specific paths relative to the CMAKE_SOURCE_DIR, before
# searching the normal paths that CMake searches
#
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PATH_SUFFIX lib/debug/)
else()
    set(PATH_SUFFIX lib/release/)
endif()

IF(ANDROID)
    MESSAGE("Searching: ${CMAKE_SOURCE_DIR}/libraries/android-${ANDROID_ABI}-clang/")
    SET(
       SIMULANT_SEARCH_PATHS
       ${CMAKE_SOURCE_DIR}/libraries/android-${ANDROID_ABI}-clang/
    )
ELSEIF(PLATFORM_DREAMCAST)
    SET(
       SIMULANT_SEARCH_PATHS
       ${CMAKE_SOURCE_DIR}/libraries/dreamcast-sh4-gcc/
    )
ELSEIF(PLATFORM_PSP)
    SET(
       SIMULANT_SEARCH_PATHS
       ${CMAKE_SOURCE_DIR}/libraries/psp-mips-gcc/
    )
ELSEIF(PLATFORM_EVERCADE)
    SET(
       SIMULANT_SEARCH_PATHS
       ${CMAKE_SOURCE_DIR}/libraries/evercade-armv7-gcc/
    )
ELSEIF(PLATFORM_RASPBERRYPI)
    SET(
       SIMULANT_SEARCH_PATHS
       ${CMAKE_SOURCE_DIR}/libraries/raspberrypi-armv7-gcc/
    )
ELSEIF(WIN32)
    SET(
       SIMULANT_SEARCH_PATHS
       ${CMAKE_SOURCE_DIR}/libraries/windows-x64-gcc/
    )
ELSE()
    IF(CMAKE_COMPILER_IS_GNUCXX)
        IF(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10)
            SET(
                SIMULANT_SEARCH_PATHS
                ${CMAKE_SOURCE_DIR}/libraries/linux-x64-gcc11/
            )
        ELSE()
            SET(
                SIMULANT_SEARCH_PATHS
                ${CMAKE_SOURCE_DIR}/libraries/linux-x64-gcc/
            )
        ENDIF()
    ENDIF()
ENDIF()

MESSAGE("${CMAKE_FIND_ROOT_PATH}")


FIND_PATH(SIMULANT_INCLUDE_DIR simulant/simulant.h PATH_SUFFIXES include PATHS ${SIMULANT_SEARCH_PATHS})
FIND_LIBRARY(SIMULANT_LIBRARY NAMES simulant PATH_SUFFIXES ${PATH_SUFFIX} PATHS ${SIMULANT_SEARCH_PATHS} NO_CACHE)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Simulant REQUIRED_VARS SIMULANT_LIBRARY SIMULANT_INCLUDE_DIR)

