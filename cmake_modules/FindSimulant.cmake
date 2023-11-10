# This module defines SIMULANT_FOUND, SIMULANT_LIBRARY AND SIMULANT_INCLUDE_DIR
#
# It prioritises architecture-specific paths relative to the CMAKE_SOURCE_DIR, before
# searching the normal paths that CMake searches
#
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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIB_SEARCH_PATHS lib lib/debug)
else()
    set(LIB_SEARCH_PATHS lib lib/release)
endif()

FIND_PATH(SIMULANT_INCLUDE_DIR simulant/simulant.h PATH_SUFFIXES include PATHS ${SIMULANT_SEARCH_PATHS})
FIND_LIBRARY(SIMULANT_LIBRARY NAMES simulant PATH_SUFFIXES ${LIB_SEARCH_PATHS} PATHS ${SIMULANT_SEARCH_PATHS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Simulant REQUIRED_VARS SIMULANT_LIBRARY SIMULANT_INCLUDE_DIR)

