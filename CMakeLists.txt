CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
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


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -DTIXML_USE_STL -Wdelete-non-virtual-dtor")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")
SET(CMAKE_ASM_FLAGS "")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -s -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -s -fomit-frame-pointer")

IF(NOT DREAMCAST_BUILD)
    IF(ANDROID)
        LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/libs/${ANDROID_ABI})

        SET(SDL2_LIBRARY SDL2)
        SET(SDL2_INCLUDE_DIR ${CMAKE_BINARY_DIR}/sdl/include)

        SET(OPENAL_LIBRARY openal)
        SET(OPENAL_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/openal/OpenAL/include)

        FIND_LIBRARY(OPENGL_LIBRARIES NAMES GLESv2)
        FIND_PATH(OPENGL_INCLUDE_DIRS NAMES gl2.h PATH_SUFFIXES GLES2)
    ELSE()
        FIND_PACKAGE(SDL2 REQUIRED)
        FIND_PACKAGE(SDL2 REQUIRED)
        FIND_PACKAGE(OpenAL REQUIRED)
        FIND_PACKAGE(OpenGL REQUIRED)
    ENDIF()

    FIND_PACKAGE(ZLIB REQUIRED)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    # Force the new C++ ABI on GCC
    MESSAGE("-- Enabling old C++11 ABI (until TravisCI is upgraded past Trusty)")
    ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=0)
ELSE()
    MESSAGE("-- Cross-Compiling for the Sega Dreamcast. Forcing Release build.")
    SET(CMAKE_BUILD_TYPE Release)

    SET(OPENAL_INCLUDE_DIRS "") # Don't use OpenAL on DC
    SET(OPENAL_LIBRARY "")

    SET(SDL2_INCLUDE_DIR "") # Don't use SDL2 on DC
    SET(SDL2_LIBRARY "")

    SET(ZLIB_INCLUDE_DIRS $ENV{KOS_PORTS}/include/zlib)
    SET(ZLIB_LIBRARIES z)

    # We build libGL source files into Simulant directly (see simulant/CMakeLists.txt)
    SET(OPENGL_INCLUDE_DIRS "")
    SET(OPENGL_LIBRARIES "")
ENDIF()

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/materials DESTINATION ${CMAKE_BINARY_DIR}/simulant)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts DESTINATION ${CMAKE_BINARY_DIR}/simulant)

INCLUDE_DIRECTORIES(
    ${SDL2_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)
IF(NOT MINGW)
    LINK_LIBRARIES(
        ${SDL2_LIBRARY}
        ${OPENAL_LIBRARY}
    )
ENDIF()
LINK_LIBRARIES( ${ZLIB_LIBRARIES} ${OPENGL_LIBRARIES})


IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
LINK_LIBRARIES(rt)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
LINK_LIBRARIES("-framework CoreFoundation")
ENDIF()

IF(ANDROID)
LINK_LIBRARIES(log)
ENDIF()

ADD_SUBDIRECTORY(simulant)
ADD_SUBDIRECTORY(tests)

IF(SIMULANT_BUILD_SAMPLES)
    ADD_SUBDIRECTORY(samples)

    IF(DREAMCAST_BUILD)
    IF(SIMULANT_BUILD_SAMPLE_CDI)
        SET(DC_EXECUTABLES # Names without .elf extension
            tests/simulant_tests
            samples/nehe02
            samples/nehe03
            samples/2d_sample
            samples/fleets_demo
            samples/light_sample
            samples/physics_demo
            samples/q2bsp_sample
            samples/rtt_sample
            samples/sample
            samples/selection_sample
            samples/terrain_demo
            samples/ui_demo
            samples/viewport_sample
            samples/particles
        )

        FOREACH(EXE ${DC_EXECUTABLES})
        get_filename_component(EXE_FILENAME ${EXE} NAME)
        ADD_CUSTOM_TARGET(
            ${EXE_FILENAME}.bin ALL
            COMMAND $ENV{KOS_OBJCOPY} -R .stack -O binary -S -g ${CMAKE_BINARY_DIR}/${EXE}.elf ${CMAKE_BINARY_DIR}/${EXE}.bin
            COMMAND rm -rf ${CMAKE_BINARY_DIR}/${EXE}
            COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${EXE}
            COMMAND $ENV{KOS_BASE}/utils/scramble/scramble ${CMAKE_BINARY_DIR}/${EXE}.bin ${CMAKE_BINARY_DIR}/${EXE}/1ST_READ.BIN
            COMMAND IP_TEMPLATE_FILE=${CMAKE_SOURCE_DIR}/platforms/dreamcast/IP.TMPL $ENV{KOS_BASE}/utils/makeip/makeip ${CMAKE_SOURCE_DIR}/platforms/dreamcast/ip.txt ${CMAKE_BINARY_DIR}/${EXE}/IP.BIN
            COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/fonts ${CMAKE_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/materials ${CMAKE_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/particles ${CMAKE_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/textures ${CMAKE_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/samples/data ${CMAKE_BINARY_DIR}/${EXE}/sample_data
            COMMAND mkisofs -C 0,11702 -V ${EXE_FILENAME} -G ${CMAKE_BINARY_DIR}/${EXE}/IP.BIN -joliet -rock -l -o ${EXE}.iso ${CMAKE_BINARY_DIR}/${EXE}
            COMMAND $ENV{KOS_BASE}/utils/img4dc/build/cdi4dc/cdi4dc ${CMAKE_BINARY_DIR}/${EXE}.iso ${CMAKE_BINARY_DIR}/${EXE}.cdi
            DEPENDS ${EXE}
            COMMENT "Converting ${EXE}.elf to ${EXE}.bin"
        )
        ENDFOREACH()
    ENDIF()
    ENDIF()
ENDIF()


## Add `make uninstall` command

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

