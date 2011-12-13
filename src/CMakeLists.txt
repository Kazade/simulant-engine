CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(kglt_sample)

FIND_PACKAGE(PkgConfig)

ADD_DEFINITIONS(-Wall -std=c++0x)

PKG_CHECK_MODULES(SDL QUIET sdl)
PKG_CHECK_MODULES(GL QUIET gl)
PKG_CHECK_MODULES(GLU QUIET glu)

SET(KGLT_SOURCES
    GL/window.cpp
    GL/object.cpp
    GL/scene.cpp
    GL/mesh.cpp
    GL/renderer.cpp
    GL/camera.cpp
    GL/texture.cpp
    GL/loader.cpp
    GL/shortcuts.cpp
    GL/loaders/q2bsp_loader.cpp
    GL/loaders/texture_loader.cpp
)

SET(SAMPLE_SOURCES
    sample.cpp
)

SET(Q2_SAMPLE_SOURCES
    q2bsp_sample.cpp
)

INCLUDE_DIRECTORIES(
    ${SDL_INCLUDE_DIRS}
    ${GL_INCLUDE_DIRS}
    ${GLU_INCLUDE_DIRS}
)

LINK_LIBRARIES(
    ${SDL_LIBRARIES}
    ${GL_LIBRARIES}
    ${GLU_LIBRARIES}
    SOIL
)

ADD_EXECUTABLE(
    kglt_sample
    ${KGLT_SOURCES}
    ${SAMPLE_SOURCES}
)

ADD_EXECUTABLE(
    kglt_bsp_sample
    ${KGLT_SOURCES}
    ${Q2_SAMPLE_SOURCES}
)
