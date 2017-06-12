FILE(GLOB_RECURSE SIMULANT_FILES FOLLOW_SYMLINKS *.cpp *.c *.cc)

# Remove all the renderer files so we can add them specifically
FILE(GLOB_RECURSE TO_REMOVE
    "renderers/gl1x/*"
    "renderers/gl2x/*"
    "deps/SOIL/original/*"
    "sdl2_window.*"
    "kos_window.*"
    "sound_drivers/*"
)

LIST(REMOVE_ITEM SIMULANT_FILES ${TO_REMOVE})

# Add GL1 files if we're targetting that
IF(SIMULANT_GL_VERSION_1X)
    MESSAGE("-- Using the OpenGL 1.1 renderer (Retro!)")

    FILE(GLOB_RECURSE GL1_FILES renderers/gl1x/*.cpp)
    SET(SIMULANT_FILES ${SIMULANT_FILES} ${GL1_FILES})

    IF(NOT DREAMCAST_BUILD)
        # DC build doesn't need glad, GL1 is available directly
        FILE(GLOB_RECURSE GLAD1_FILES renderers/gl1x/glad/*.c)
        SET(SIMULANT_FILES ${SIMULANT_FILES} ${GLAD1_FILES})
    ENDIF()
ENDIF()

# Add GL2 files if we're using that instead
IF(SIMULANT_GL_VERSION_2X)
    MESSAGE("-- Using the OpenGL 2.1 renderer (Old Skool!)")

    FILE(GLOB_RECURSE GL2_FILES renderers/gl2x/*.c*)
    SET(SIMULANT_FILES ${SIMULANT_FILES} ${GL2_FILES})
ENDIF()

IF(DREAMCAST_BUILD)
    SET(SIMULANT_FILES ${SIMULANT_FILES} kos_window.cpp sound_drivers/kos_sound_driver.cpp)
ELSE()
    SET(SIMULANT_FILES ${SIMULANT_FILES} sdl2_window.cpp sound_drivers/openal_sound_driver.cpp sound_drivers/al_error.cpp)
ENDIF()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/bounce/include)

IF(DREAMCAST_BUILD)
ADD_LIBRARY(simulant STATIC ${SIMULANT_FILES})
ELSE()
ADD_LIBRARY(simulant SHARED ${SIMULANT_FILES})
ENDIF()

SET_TARGET_PROPERTIES(simulant
                      PROPERTIES
                      VERSION 0.0.1
                      SOVERSION 1)

INSTALL(TARGETS simulant DESTINATION lib)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.inl" PATTERN "deps*" EXCLUDE)

SET(DEPS
    "jsonic"
    "kazlog"
    "kazsignal"
    "kfs"
    "SOIL"
    "tinyxml"
    "stb_truetype"
)

FOREACH(DEP ${DEPS})
    SET(DEP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/${DEP}")
    SET(DEP_INSTALL_PATH include/simulant/deps/${DEP})
    FILE(GLOB DEP_INCLUDES FOLLOW_SYMLINKS ${DEP_PATH}/*.h ${DEP_PATH}/*.inl)
    INSTALL(DIRECTORY DESTINATION ${DEP_INSTALL_PATH})
    INSTALL(FILES ${DEP_INCLUDES} DESTINATION ${DEP_INSTALL_PATH})
ENDFOREACH()

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/simulant-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/opengl-2.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials/opengl-2.x)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/opengl-1.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials/opengl-1.x)

FILE(GLOB textures "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.png")
INSTALL(FILES ${textures} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.vert")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.frag")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials)

FILE(GLOB particles "${CMAKE_CURRENT_SOURCE_DIR}/particles/*.kglp")
INSTALL(FILES ${particles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/particles)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/ubuntu/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/fonts/ubuntu)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/opensans/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/fonts/opensans)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/fontawesome/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/fonts/fontawesome)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/probe/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/fonts/probe)
