FILE(GLOB_RECURSE KGLT_HEADERS *h)
FILE(GLOB_RECURSE KGLT_FILES FOLLOW_SYMLINKS *.cpp *.c *.cc)

# Remove all the renderer files so we can add them specifically
FILE(GLOB_RECURSE TO_REMOVE "renderers/gl1x/*" "renderers/gl2x/*" "deps/SOIL/original/*")
LIST(REMOVE_ITEM KGLT_FILES ${TO_REMOVE})

# Add GL1 files if we're targetting that
IF(KGLT_GL_VERSION_1X)
    MESSAGE("-- Using the OpenGL 1.1 renderer (Retro!)")

    FILE(GLOB_RECURSE GL1_FILES renderers/gl1x/*.c*)
    SET(KGLT_FILES ${KGLT_FILES} ${GL1_FILES})
ENDIF()

# Add GL2 files if we're using that instead
IF(KGLT_GL_VERSION_2X)
    MESSAGE("-- Using the OpenGL 2.1 renderer (Old Skool!)")

    FILE(GLOB_RECURSE GL2_FILES renderers/gl2x/*.c*)
    SET(KGLT_FILES ${KGLT_FILES} ${GL2_FILES})
ENDIF()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)

ADD_LIBRARY(kglt SHARED ${KGLT_FILES})
SET_TARGET_PROPERTIES(kglt
                      PROPERTIES
                      VERSION 0.0.1
                      SOVERSION 1)

INSTALL(TARGETS kglt DESTINATION lib)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/kglt-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/opengl-2.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials/opengl-2.x)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/opengl-1.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials/opengl-1.x)

FILE(GLOB textures "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.png")
INSTALL(FILES ${textures} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.vert")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.frag")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB particles "${CMAKE_CURRENT_SOURCE_DIR}/particles/*.kglp")
INSTALL(FILES ${particles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/particles)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/ubuntu/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/ubuntu)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/opensans/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/opensans)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/fontawesome/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/fontawesome)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/probe/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/probe)
