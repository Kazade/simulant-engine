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
)

SET(SOURCES
    sample.cpp
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
)

ADD_EXECUTABLE(
	kglt_sample
        ${KGLT_SOURCES}
	${SOURCES}
)
