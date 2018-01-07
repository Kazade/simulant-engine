cmake_minimum_required(VERSION 2.8)
project(__project_name_pascal__)

SET(SIMULANT_INCLUDE_FOLDER "" CACHE STRING "Specify the path to the Simulant includes")
SET(SIMULANT_LIBRARY_FOLDER "" CACHE STRING "Specify the path to the Simulant libraries")
SET(USE_GLOBAL_SIMULANT TRUE)

# If an include override was passed, use that
IF(NOT ${SIMULANT_INCLUDE_FOLDER} MATCHES "^$")
    INCLUDE_DIRECTORIES(BEFORE ${SIMULANT_INCLUDE_FOLDER})
    SET(USE_GLOBAL_SIMULANT FALSE)
ENDIF()

# If a library override was passed, use that
IF(NOT ${SIMULANT_LIBRARY_FOLDER} MATCHES "^$")
    LINK_DIRECTORIES(${SIMULANT_LIBRARY_FOLDER})
    LINK_LIBRARIES(simulant)
    SET(USE_GLOBAL_SIMULANT_FALSE)
ENDIF()

IF(${USE_GLOBAL_SIMULANT})
    # If we were missing either override, use the cmake module
    FIND_PACKAGE(SIMULANT)
    INCLUDE_DIRECTORIES(${SIMULANT_INCLUDE_DIRS})
    LINK_LIBRARIES(${SIMULANT_LIBRARY})
    MESSAGE("-- Using Simulant from OS")
ENDIF()

IF(${DREAMCAST_BUILD)
    INCLUDE_DIRECTORIES(
        $ENV{KOS_PORTS}/include/libGL
        $ENV{KOS_PORTS}/include/zlib
    )
    LINK_LIBRARIES(GL z m)
ELSE()
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
    PKG_CHECK_MODULES(GL REQUIRED gl)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    # Simulant is compiled with the old GCC API (for now)
    ADD_DEFINITIONS(-D_GLIBCXX_USE_CXX11_ABI=0)

    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${GL_INCLUDE_DIRS})
    LINK_LIBRARIES(${SDL2_LIBRARIES} ${GL_LIBRARIES})
ENDIF()

FILE(GLOB_RECURSE SOURCES FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/sources/*.cpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions("-Wall")
add_executable(__project_name_lower__ ${SOURCES})

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)
