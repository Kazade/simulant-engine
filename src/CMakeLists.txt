CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(kglt_sample)

FIND_PACKAGE(PkgConfig)

ADD_DEFINITIONS(-Wall -std=c++0x -g)

PKG_CHECK_MODULES(SDL QUIET sdl)
PKG_CHECK_MODULES(GL QUIET gl)
PKG_CHECK_MODULES(GLU QUIET glu)
PKG_CHECK_MODULES(SIGC QUIET sigc++-2.0)

FIND_PACKAGE(Boost COMPONENTS system filesystem thread date_time REQUIRED)

SET(KGLT_SOURCES
    kglt/window.cpp
    kglt/object.cpp
    kglt/scene.cpp
    kglt/mesh.cpp
    kglt/renderer.cpp
    kglt/camera.cpp
    kglt/texture.cpp
    kglt/loader.cpp
    kglt/viewport.cpp
    kglt/shader.cpp
    kglt/shortcuts.cpp
    kglt/loaders/q2bsp_loader.cpp
    kglt/loaders/texture_loader.cpp
    kglt/utils/gl_error.cpp
    glee/GLee.c
)

SET(KAZBASE_SOURCES
    kazbase/logging/logging.cpp
)


SET(KAZMATH_SOURCES
    kazmath/vec2.c
    kazmath/vec3.c    
    kazmath/vec4.c    
    kazmath/mat3.c    
    kazmath/mat4.c    
    kazmath/plane.c    
    kazmath/aabb.c    
    kazmath/utility.c    
    kazmath/ray2.c    
    kazmath/quaternion.c    
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
    ${SIGC_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

LINK_LIBRARIES(
    ${SDL_LIBRARIES}
    ${GL_LIBRARIES}
    ${GLU_LIBRARIES}
    ${SIGC_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    SOIL
)

ADD_EXECUTABLE(
    kglt_sample
    ${KGLT_SOURCES}
    ${KAZMATH_SOURCES}
    ${KAZBASE_SOURCES}
    ${SAMPLE_SOURCES}
)

ADD_EXECUTABLE(
    kglt_bsp_sample
    ${KGLT_SOURCES}
    ${KAZMATH_SOURCES}    
    ${KAZBASE_SOURCES}    
    ${Q2_SAMPLE_SOURCES}
)
