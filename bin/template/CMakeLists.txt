cmake_minimum_required(VERSION 2.8)
project(__project_name_pascal__)

SET(SIMULANT_INCLUDE_FOLDER "" CACHE STRING "Specify the path to the Simulant includes")
SET(SIMULANT_LIBRARY_FOLDER "" CACHE STRING "Specify the path to the Simulant libraries")

FIND_PACKAGE(PkgConfig REQUIRED)

# If an include override was passed, use that
IF(NOT ${SIMULANT_INCLUDE_FOLDER} STREQUAL "")
    INCLUDE_DIRECTORIES(BEFORE ${SIMULANT_INCLUDE_FOLDER})
ENDIF()

# If a library override was passed, use that
IF(NOT ${SIMULANT_LIBRARY_FOLDER} STREQUAL "")
    LINK_DIRECTORIES(${SIMULANT_LIBRARY_FOLDER})
    LINK_LIBRARIES(simulant)
ENDIF()

IF(${SIMULANT_INCLUDE_FOLDER} STREQUAL "")
    # If we were missing either override, use the cmake module
    FIND_PACKAGE(SIMULANT)
    INCLUDE_DIRECTORIES(${SIMULANT_INCLUDE_DIRS})
    LINK_LIBRARIES(${SIMULANT_LIBRARIES})
ENDIF()

PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
PKG_CHECK_MODULES(GL REQUIRED gl)

INCLUDE_DIRECTORIES(
    ${SDL2_INCLUDE_DIRS}
    ${GL_INCLUDE_DIRS}
)

LINK_LIBRARIES(
    ${SDL2_LIBRARIES}
    ${GL_LIBRARIES}
)

FILE(GLOB_RECURSE SOURCES FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/src/*.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

add_definitions("-Wall -g")
add_executable(__project_name_lower__ ${SOURCES})

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)
