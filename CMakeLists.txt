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
IF(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    OPTION(SIMULANT_BUILD_TESTS "Build Simulant tests" OFF)
ELSE()
    OPTION(SIMULANT_BUILD_TESTS "Build Simulant tests" ON)
ENDIF()

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
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIXML_USE_STL")

IF(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /W3")
ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wpedantic -Wextra -Wno-variadic-macros")
ENDIF()

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdouble-promotion")
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -faligned-new")
ENDIF()

IF(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall")
ELSE()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c99 /W3")
ENDIF()

SET(CMAKE_ASM_FLAGS "")

# If we're forcing profiling mode, set that define
IF(SIMULANT_PROFILE)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSIMULANT_PROFILE")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIMULANT_PROFILE")
ENDIF()

INCLUDE(CheckFunctionExists)
check_function_exists("pthread_yield" HAS_PTHREAD_YIELD)
IF(${HAS_PTHREAD_YIELD})
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSIMULANT_HAS_PTHREAD_YIELD")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIMULANT_HAS_PTHREAD_YIELD")
ENDIF()

INCLUDE(CheckIPOSupported)
check_ipo_supported(RESULT FLTO_SUPPORTED OUTPUT FLTO_ERROR)

# Mingw has a bug that stops us compiling using LTO
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94271
IF(MINGW)
    SET(FLTO_SUPPORTED False)
ENDIF()

# We force disable debugging information in release builds - this is so we override settings in the
# kallistios gnu-wrappers which sometimes pick up -g through the default env vars
# Fast-math is important on DC!
IF(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fomit-frame-pointer -O3 -g0 -ffast-math -finline-functions")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer -O3 -g0 -ffast-math -finline-functions")
ENDIF()

IF(PLATFORM_DREAMCAST)
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mfsrra -mfsca -ffp-contract=fast")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mfsrra -mfsca -ffp-contract=fast")
ENDIF()

IF(NOT PLATFORM_PSP)
    IF(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    ELSE()
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:NONE")
    ENDIF()
ENDIF()

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # The /Zi option produces a separate PDB file that contains all the symbolic debugging information for use with the debugger. 
    # The debugging information isn't included in the object files or executable, which makes them much smaller.
    add_compile_options("$<$<CONFIG:Debug>:/Zi>")

    # This option enables a token-based preprocessor that conforms to C99 and C++11 and later standards.
    add_compile_options("$<$<CONFIG:Release>:/Zc:preprocessor>")

    # Disables frame pointer omission
    add_compile_options("$<$<CONFIG:Debug>:/Oy->")

    # Enable incremental linking in debug builds
    add_compile_options("$<$<CONFIG:Debug>:/INCREMENTAL>")

    # This should reduce the binary size in release builds
    add_compile_options("$<$<CONFIG:Release>:/LTCG>")

    # Enable full optimization for release builds
    add_compile_options("$<$<CONFIG:Release>:/Ox /Oy /fp:fast>")

    # Enables unwind semantic for exception handler in debug builds
    add_compile_options("$<$<CONFIG:Debug>:/EHsc>")

    # Add UTF-8 support (all builds)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
ENDIF()


IF(SIMULANT_ENABLE_ASAN)
IF(CMAKE_BUILD_TYPE MATCHES Debug AND NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
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
        SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
        link_libraries(${ASAN_LIBRARY})
    ENDIF()
ENDIF()
ENDIF()

IF(SIMULANT_ENABLE_TSAN)
IF(CMAKE_BUILD_TYPE MATCHES Debug AND NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message("Enabling thread sanitizer")
    SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
ENDIF()
ENDIF()

SET(USE_GLAD_FOR_OPENGL TRUE)

IF(PLATFORM_DREAMCAST)
    SET(USE_GLAD_FOR_OPENGL FALSE)
ELSEIF(PLATFORM_PSP)
    SET(USE_GLAD_FOR_OPENGL FALSE)
ENDIF()

IF(USE_GLAD_FOR_OPENGL)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSIMULANT_USE_GLAD")
ENDIF()

IF(ANDROID)
    include_directories(
        ${ANDROID_NDK}/sources/android/native_app_glue
    )

    add_library(
        openal
        SHARED
        IMPORTED
    )

    set_target_properties(
        openal
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/platforms/android/openal/lib/${ANDROID_ABI}/libopenal.so
    )

    add_library(
        native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )
    link_libraries(android openal)

    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    SET(OPENGL_LIBRARIES GLESv2 EGL)
    FIND_PATH(OPENGL_INCLUDE_DIRS NAMES gl2.h PATH_SUFFIXES GLES2)
    SET(OPENAL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/platforms/android/openal/include)

ELSEIF(PLATFORM_PSP)
    SET(SDL2_INCLUDE_DIR "") # Don't use SDL2 on PSP
    SET(SDL2_LIBRARIES "")
ELSEIF(MINGW)
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
    PKG_CHECK_MODULES(OPENAL REQUIRED openal)
    LINK_LIBRARIES(psapi userenv)
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    add_definitions(-D_WIN32_WINNT=0x0601 -D_WINSOCK_DEPRECATED_NO_WARNINGS)
ELSEIF(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # On Windows, SDL2 and OpenAL need to be installed and the paths need to be correctly set
    # For SDL2 it's the path under SDL2DIR
    # For OpenAL it's the path under OPENALDIR
    IF(NOT SDL2_FOUND)
        SET(SDL2_INCLUDE_DIR $ENV{SDL2DIR}/include)
        SET(SDL2_LIBRARY $ENV{SDL2DIR}/lib/x64/SDL2.lib)
        FIND_PACKAGE(SDL2 REQUIRED)
    ENDIF()

    IF(NOT SIMULANT_USE_ALDC)
        SET(OPENAL_INCLUDE_DIR $ENV{OPENALDIR}/include)
        SET(OPENAL_INCLUDE_DIRS $ENV{OPENALDIR}/include)
        SET(OPENAL_LIBRARY $ENV{OPENALDIR}/libs/Win64/OpenAL32.lib)
    ENDIF()

    LINK_LIBRARIES(psapi userenv)

    # Maybe of use for dynamic linking (also specify /MD and /MDd)
    # __declspec(dllexport) is missing everywhere, so that's not working right now
    #SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")  
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")

    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

    add_definitions(-D__WIN32__ -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)

    # Needed to allow our custom min max implementations
    add_definitions(-DNOMINMAX)

    # Disable warnings
    # Signed / unsigned mismatch
    add_definitions("/wd4018")

    # conditional expression is constant
    add_definitions("/wd4127")

    # nonstandard extension used : nameless struct/union
    add_definitions("/wd4201")

    #'class1' : inherits 'class2::member' via dominance
    add_definitions("/wd4250")

    # relative include path contains '..'
    add_definitions("/wd4464")

    # decorated name length exceeded, name was truncated
    add_definitions("/wd4503")

    # assignment operator could not be generated
    add_definitions("/wd4512")

    # C++ exception handler used, but unwind semantics are not enabled (EHsc)
    add_definitions("/wd4530")

    # 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed
    add_definitions("/wd4577")

    # conversion from 'type_1' to 'type_2' requires a narrowing conversion
    add_definitions("/wd4838")

    # _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead
    add_definitions("/wd6255")

    # Enable warnings
    # unreferenced local variable
    add_definitions("/w34101")

    # local variable is initialized but not referenced
    add_definitions("/w34189")

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
    SET(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    link_libraries(Threads::Threads)
ENDIF()


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
    ${OPENAL_LIBRARY}
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

FILE(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)
ADD_SUBDIRECTORY(simulant)

IF(SIMULANT_BUILD_TESTS)
    ENABLE_TESTING()
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
