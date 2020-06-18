CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
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

IF(DREAMCAST_BUILD)
OPTION(SIMULANT_SEPERATE_DEBUGINFO "Generate debuginfo seperately and strip from executable" ON)
ELSE()
OPTION(SIMULANT_SEPERATE_DEBUGINFO "Generate debuginfo seperately and strip from executable" OFF)
ENDIF()

SET(OpenGL_GL_PREFERENCE "LEGACY")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -DTIXML_USE_STL -Wdouble-promotion")
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

# We force disable debugging information in release builds - this is so we override settings in the
# kallistios gnu-wrappers which sometimes pick up -g through the default env vars
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -fomit-frame-pointer -g0 -ffast-math")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -fomit-frame-pointer -g0 -ffast-math")

IF(NOT DREAMCAST_BUILD)
# There's a bug in GCC 4.7.4 that causes an internal compiler error
# on some template code if you have -Wextra enabled
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra")
ELSE()
# On the Dreamcast, force a frame pointer when debugging
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")
ENDIF()

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

IF(NOT DREAMCAST_BUILD)
    IF(ANDROID)
        LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/.android/sdl/build/libs/${ANDROID_ABI})
        LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/.android/openal/build/libs/${ANDROID_ABI})

        SET(SDL2_LIBRARIES SDL2)
        SET(SDL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/.android/sdl/include)

        SET(OPENAL_LIBRARY openal)
        SET(OPENAL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/.android/openal/OpenAL/include)

        FIND_LIBRARY(OPENGL_LIBRARIES NAMES GLESv2)
        FIND_PATH(OPENGL_INCLUDE_DIRS NAMES gl2.h PATH_SUFFIXES GLES2)
    ELSE()
        FIND_PACKAGE(SDL2)  # Try cmake modules first
        IF(NOT SDL2_FOUND)
            # Fall back to pkg-config
            FIND_PACKAGE(PkgConfig REQUIRED)
            PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
        ENDIF()

        IF(WIN32)
            IF(NOT SDL2_FOUND)
                SET(SDL2_INCLUDE_DIR $ENV{SDL2DIR}/include)
                SET(SDL2_LIBRARY $ENV{SDL2DIR}/lib/x64/SDL2.lib)
                FIND_PACKAGE(SDL2 REQUIRED)
            ENDIF()

            SET(OPENAL_INCLUDE_DIR $ENV{OPENALDIR}/include)
            SET(OPENAL_INCLUDE_DIRS $ENV{OPENALDIR}/include)

            SET(ZLIB_INCLUDE_DIR $ENV{ZLIBDIR}/include)
        ENDIF()

        FIND_PACKAGE(OpenAL REQUIRED)
        FIND_PACKAGE(OpenGL REQUIRED)
    ENDIF()

    FIND_PACKAGE(ZLIB REQUIRED)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    link_libraries(Threads::Threads)

    # Force the new C++ ABI on GCC
    MESSAGE("-- Enabling old C++11 ABI")
    ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=0)
ELSE()
    MESSAGE("-- Cross-Compiling for the Sega Dreamcast.")

    SET(OPENAL_INCLUDE_DIRS "")
    SET(OPENAL_LIBRARY "")

    SET(SDL2_INCLUDE_DIR "") # Don't use SDL2 on DC
    SET(SDL2_LIBRARIES "")

    SET(ZLIB_INCLUDE_DIRS $ENV{KOS_PORTS}/include/zlib)
    SET(ZLIB_LIBRARIES z)

    # We build libGL source files into Simulant directly (see simulant/CMakeLists.txt)
    SET(OPENGL_INCLUDE_DIRS "")
    SET(OPENGL_LIBRARIES "")

    SET(THREADS_PREFER_PTHREAD_FLAG OFF)

    FILE(GENERATE OUTPUT ${CMAKE_SOURCE_DIR}/deps/libgl/GL/version.h CONTENT "#pragma once\n#define GLDC_VERSION \"1.1-custom\"\n")
ENDIF()

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/materials DESTINATION ${CMAKE_BINARY_DIR}/simulant)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts DESTINATION ${CMAKE_BINARY_DIR}/simulant)

INCLUDE_DIRECTORIES(
    ${SDL2_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

LINK_LIBRARIES(${SDL2_LIBRARIES} ${SDL2_LIBRARY} ${OPENAL_LIBRARY} ${ZLIB_LIBRARIES} ${OPENGL_LIBRARIES})


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

