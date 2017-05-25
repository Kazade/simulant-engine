CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(simulant)

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
OPTION(SIMULANT_GL_VERSION_1X "Enable GL 1.x support" OFF)

IF(SIMULANT_GL_VERSION_1X OR DREAMCAST_BUILD)
    # Dreamcast builds always use this
    SET(SIMULANT_GL_VERSION_1X "1")
    ADD_DEFINITIONS(-DSIMULANT_GL_VERSION_1X)
ELSE()
    SET(SIMULANT_GL_VERSION_2X "1")
    ADD_DEFINITIONS(-DSIMULANT_GL_VERSION_2X)
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

ADD_DEFINITIONS("-Wall -DTIXML_USE_STL")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -s -fomit-frame-pointer")

IF(DREAMCAST_BUILD)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFRAME_POINTERS -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFRAME_POINTERS -fno-omit-frame-pointer")
ENDIF()

IF(NOT DREAMCAST_BUILD) 
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(SDL2 sdl2 REQUIRED)
    FIND_PACKAGE(OpenAL REQUIRED)
    FIND_PACKAGE(ZLIB REQUIRED)
    FIND_PACKAGE(OpenGL REQUIRED)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ELSE()
    MESSAGE("COMPILING FOR DREAMCAST")

    SET(OPENAL_INCLUDE_DIRS "") # Don't use OpenAL on DC
    SET(OPENAL_LIBRARY "")

    SET(SDL2_INCLUDE_DIRS "") # Don't use SDL2 on DC
    SET(SDL2_LIBRARIES "")

    SET(ZLIB_INCLUDE_DIRS $ENV{KOS_PORTS}/include/zlib)
    SET(ZLIB_LIBRARIES z)

    SET(OPENGL_INCLUDE_DIRS $ENV{KOS_PORTS}/include/libGL)
    SET(OPENGL_LIBRARIES GL)
ENDIF()

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/simulant/materials DESTINATION ${CMAKE_BINARY_DIR}/simulant)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/simulant/fonts DESTINATION ${CMAKE_BINARY_DIR}/simulant)

INCLUDE_DIRECTORIES(
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

LINK_LIBRARIES(
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${ZLIB_LIBRARIES}
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
LINK_LIBRARIES(rt)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
LINK_LIBRARIES("-framework CoreFoundation")
ENDIF()

ADD_SUBDIRECTORY(simulant)
ADD_SUBDIRECTORY(tests)

IF(SIMULANT_BUILD_SAMPLES)
    ADD_SUBDIRECTORY(samples)
ENDIF(SIMULANT_BUILD_SAMPLES)

IF(DREAMCAST_BUILD)

SET(DC_EXECUTABLES # Names without .elf extension
    tests/simulant_tests
    samples/nehe01
    samples/2d_sample
    samples/fleets_demo
    samples/joypad_sample
    samples/light_sample
    samples/physics_demo
    samples/q2bsp_sample
    samples/rtt_sample
    samples/sample
    samples/selection_sample
    samples/terrain_demo
    samples/ui_demo
    samples/viewport_sample
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
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/simulant/fonts ${CMAKE_BINARY_DIR}/${EXE}/simulant
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/simulant/materials ${CMAKE_BINARY_DIR}/${EXE}/simulant
    COMMAND mkisofs -C 0,11702 -V ${EXE_FILENAME} -G ${CMAKE_BINARY_DIR}/${EXE}/IP.BIN -joliet -rock -l -o ${EXE}.iso ${CMAKE_BINARY_DIR}/${EXE}
    COMMAND $ENV{KOS_BASE}/utils/img4dc/build/cdi4dc/cdi4dc ${CMAKE_BINARY_DIR}/${EXE}.iso ${CMAKE_BINARY_DIR}/${EXE}.cdi
    DEPENDS ${EXE}
    COMMENT "Converting ${EXE}.elf to ${EXE}.bin"
)
ENDFOREACH()

ENDIF()

