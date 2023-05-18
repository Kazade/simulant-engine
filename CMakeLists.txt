CMAKE_MINIMUM_REQUIRED(VERSION 3.9)
PROJECT(simulant CXX C ASM)

# Set a default build type if none was specified
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  MESSAGE(STATUS "Setting CMAKE_BUILD_TYPE to 'Debug' by default. Use -DCMAKE_BUILD_TYPE=Release for a release build.")
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()


# Set module options
OPTION(SIMULANT_BUILD_TESTS "Build Simulant tests" ON)
OPTION(SIMULANT_BUILD_SAMPLES "Build Simulant samples" ON)
OPTION(SIMULANT_BUILD_SAMPLE_CDI "Build Dreamcast samples as CDI images" OFF)
OPTION(SIMULANT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
OPTION(SIMULANT_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
OPTION(SIMULANT_PROFILE "Force profiling mode" OFF)

# This is only for testing! You will not get any sound!
OPTION(SIMULANT_USE_ALDC "Use ALdc on all platforms" OFF)

IF(PLATFORM_DREAMCAST)
OPTION(SIMULANT_SEPERATE_DEBUGINFO "Generate debuginfo seperately and strip from executable" ON)
ELSE()
OPTION(SIMULANT_SEPERATE_DEBUGINFO "Generate debuginfo seperately and strip from executable" OFF)
ENDIF()

SET(OpenGL_GL_PREFERENCE "LEGACY")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -DTIXML_USE_STL -Wdouble-promotion")

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -faligned-new")
endif()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall")
SET(CMAKE_ASM_FLAGS "")

# If we're forcing profiling mode, set that define
IF(SIMULANT_PROFILE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSIMULANT_PROFILE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIMULANT_PROFILE")
ENDIF()

include(CheckFunctionExists)
check_function_exists("pthread_yield" HAS_PTHREAD_YIELD)
IF(${HAS_PTHREAD_YIELD})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSIMULANT_HAS_PTHREAD_YIELD")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIMULANT_HAS_PTHREAD_YIELD")
ENDIF()

include(CheckIPOSupported)
check_ipo_supported(RESULT FLTO_SUPPORTED OUTPUT FLTO_ERROR)

# Mingw has a bug that stops us compiling using LTO
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94271
IF(MINGW)
    SET(FLTO_SUPPORTED False)
ENDIF()

# We force disable debugging information in release builds - this is so we override settings in the
# kallistios gnu-wrappers which sometimes pick up -g through the default env vars
# Fast-math is important on DC!
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fomit-frame-pointer -g0 -ffast-math")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer -g0 -ffast-math")

IF(PLATFORM_DREAMCAST)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -mfsrra -mfsca -ffast-math -ffp-contract=fast -finline-functions")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mfsrra -mfsca -ffast-math -ffp-contract=fast -finline-functions")
ENDIF()

IF(NOT PLATFORM_PSP)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
ENDIF()

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -Wextra")

IF(SIMULANT_ENABLE_ASAN)
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    # For some reason, I need to explicitly specify the shared file
    # on Fedora. This is brittle, but I don't know how best to fix it
    FIND_LIBRARY(
        ASAN_LIBRARY NAMES asan libasan.so.5 PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
    )

    message(${ASAN_LIBRARY})

    IF(NOT ASAN_LIBRARY)
        message("libasan NOT detected. Disabling address sanitizer")
    ELSE()
        message("libasan found. Enabling address sanitizer")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
        link_libraries(${ASAN_LIBRARY})
    ENDIF()
ENDIF()
ENDIF()

IF(SIMULANT_ENABLE_TSAN)
IF(CMAKE_BUILD_TYPE MATCHES Debug)
message("Enabling thread sanitizer")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
ENDIF()
ENDIF()

SET(USE_GLAD_FOR_OPENGL TRUE)

IF(PLATFORM_DREAMCAST)
    SET(USE_GLAD_FOR_OPENGL FALSE)
ELSEIF(PLATFORM_PSP)
    SET(USE_GLAD_FOR_OPENGL FALSE)
ENDIF()

IF(USE_GLAD_FOR_OPENGL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIMULANT_USE_GLAD")
ENDIF()

IF(ANDROID)
    LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/.android/sdl/build/libs/${ANDROID_ABI})
    LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/.android/openal/build/libs/${ANDROID_ABI})

    SET(SDL2_LIBRARIES SDL2)
    SET(SDL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/.android/sdl/include)

    IF(SIMULANT_USE_ALDC)

    ELSE()
        SET(OPENAL_LIBRARY openal)
        SET(OPENAL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/.android/openal/OpenAL/include)
    ENDIF()

    FIND_LIBRARY(OPENGL_LIBRARIES NAMES GLESv2)
    FIND_PATH(OPENGL_INCLUDE_DIRS NAMES gl2.h PATH_SUFFIXES GLES2)
ELSEIF(PLATFORM_PSP)
    SET(SDL2_INCLUDE_DIR "") # Don't use SDL2 on PSP
    SET(SDL2_LIBRARIES "")
ELSEIF(MINGW)
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
    PKG_CHECK_MODULES(OPENAL REQUIRED openal)
    LINK_LIBRARIES(psapi userenv)
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
ELSEIF(PLATFORM_DREAMCAST)
    MESSAGE("-- Cross-Compiling for the Sega Dreamcast.")

    SET(OPENAL_INCLUDE_DIRS "")
    SET(OPENAL_LIBRARY "")

    SET(SDL2_INCLUDE_DIR "") # Don't use SDL2 on DC
    SET(SDL2_LIBRARIES "")

    # We build libGL source files into Simulant directly (see simulant/CMakeLists.txt)
    SET(OPENGL_INCLUDE_DIRS "")
    SET(OPENGL_LIBRARIES "")

    SET(THREADS_PREFER_PTHREAD_FLAG OFF)

    # On Dreamcast we want the kospvr backend for GLdc
    add_definitions(-DBACKEND_KOSPVR)

    FILE(GENERATE OUTPUT ${CMAKE_SOURCE_DIR}/deps/libgl/GL/version.h CONTENT "#pragma once\n#define GLDC_VERSION \"1.1-custom\"\n")
ELSE()
    FIND_PACKAGE(SDL2)  # Try cmake modules first
    IF(NOT SDL2_FOUND)
        # Fall back to pkg-config
        FIND_PACKAGE(PkgConfig REQUIRED)
        PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
    ENDIF()

    IF(SIMULANT_USE_ALDC)
        SET(OPENAL_INCLUDE_DIRS "")
        SET(OPENAL_LIBRARY "")
    ELSE()
        FIND_PACKAGE(OpenAL REQUIRED)
    ENDIF()

    FIND_PACKAGE(OpenGL REQUIRED)
ENDIF()

IF(NOT PLATFORM_PSP AND NOT PLATFORM_DREAMCAST)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    link_libraries(Threads::Threads)
ENDIF()


FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/materials DESTINATION ${CMAKE_BINARY_DIR}/simulant)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts DESTINATION ${CMAKE_BINARY_DIR}/simulant)

INCLUDE_DIRECTORIES(
    ${SDL2_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${OPENAL_LIBRARY_DIRS}
    ${SDL2_LIBRARY_DIRS}
)

LINK_LIBRARIES(
    ${SDL2_LIBRARY}
    ${SDL2_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
LINK_LIBRARIES(rt)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
LINK_LIBRARIES("-framework CoreFoundation")
ENDIF()

IF(ANDROID)
LINK_LIBRARIES(log)
ENDIF()

# FIXME: Only do this if we're building samples or tests
FILE(COPY ${CMAKE_SOURCE_DIR}/samples/data/ DESTINATION ${CMAKE_BINARY_DIR}/sample_data/)
FILE(COPY ${CMAKE_SOURCE_DIR}/assets/particles/ DESTINATION ${CMAKE_BINARY_DIR}/simulant/particles/)
FILE(COPY ${CMAKE_SOURCE_DIR}/assets/materials/ DESTINATION ${CMAKE_BINARY_DIR}/simulant/materials/)
FILE(COPY ${CMAKE_SOURCE_DIR}/assets/textures/ DESTINATION ${CMAKE_BINARY_DIR}/simulant/textures/)
FILE(COPY ${CMAKE_SOURCE_DIR}/assets/fonts/ DESTINATION ${CMAKE_BINARY_DIR}/simulant/fonts/)
FILE(COPY ${CMAKE_SOURCE_DIR}/assets/sounds/ DESTINATION ${CMAKE_BINARY_DIR}/simulant/sounds/)

ADD_SUBDIRECTORY(simulant)

IF(SIMULANT_BUILD_TESTS)
    ADD_SUBDIRECTORY(tests)
ENDIF()

IF(SIMULANT_BUILD_SAMPLES)
    ADD_SUBDIRECTORY(samples)
ENDIF()


## Add `make uninstall` command

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

