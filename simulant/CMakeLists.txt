FILE(GLOB_RECURSE SIMULANT_FILES FOLLOW_SYMLINKS *.cpp *.c *.cc)

# Remove all the renderer files so we can add them specifically
FILE(GLOB_RECURSE TO_REMOVE
    "renderers/glad/*"
    "renderers/gl2x/*"
    "deps/SOIL/original/*"
    "deps/bounce_src/bounce/garment/triangle/tricall.c"
    "sdl2_window.*"
    "kos_window.*"
    "sound_drivers/*"
)

LIST(REMOVE_ITEM SIMULANT_FILES ${TO_REMOVE})


IF(NOT DREAMCAST_BUILD)
    MESSAGE("-- Including GLAD in build")

    FILE(GLOB_RECURSE GL2_FILES renderers/gl2x/*.c*)
    SET(SIMULANT_FILES ${SIMULANT_FILES} ${GL2_FILES})
    # DC build doesn't need glad, GL1 is available directly
    FILE(GLOB_RECURSE GLAD1_FILES renderers/glad/*.c)
    SET(SIMULANT_FILES ${SIMULANT_FILES} ${GLAD1_FILES})
ENDIF()


IF(DREAMCAST_BUILD)
    MESSAGE("-- Including GLdc and ALdc in build, as this is the Dreamcast")

    # We build in GLdc and ALdc directly (as it's all under the Simulant umbrella)
    FILE(GLOB_RECURSE LIBGL ${CMAKE_SOURCE_DIR}/deps/libgl/*.c)
    FILE(GLOB_RECURSE LIBGL_ASM ${CMAKE_SOURCE_DIR}/deps/libgl/*.S)

    SET(
        LIBAL
        ${CMAKE_SOURCE_DIR}/deps/libAL/AL/mojoal.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/AL/aldc.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/AL/alut.c
    )

    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/libgl/include ${CMAKE_SOURCE_DIR}/deps/libAL/include)

    SET(SIMULANT_FILES ${SIMULANT_FILES}
        ${LIBGL}
        ${LIBGL_ASM}
        ${LIBAL}
    )

    SET(SIMULANT_FILES ${SIMULANT_FILES} kos_window.cpp sound_drivers/openal_sound_driver.cpp sound_drivers/al_error.cpp sound_drivers/null_sound_driver.cpp)
ELSE()
    SET(SIMULANT_FILES ${SIMULANT_FILES} sdl2_window.cpp sound_drivers/openal_sound_driver.cpp sound_drivers/al_error.cpp sound_drivers/null_sound_driver.cpp)
ENDIF()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/bounce/include)

IF(DREAMCAST_BUILD)
ADD_LIBRARY(simulant STATIC ${SIMULANT_FILES})
ELSE()
ADD_LIBRARY(simulant SHARED ${SIMULANT_FILES})
ENDIF()

IF(MINGW)
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ ")
    SET(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ ")
    SET(CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_MODULE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ ")
    TARGET_LINK_LIBRARIES(simulant LINK_PUBLIC opengl32 glu32 mingw32 SDL2main SDL2 OpenAL32 userenv)
ENDIF()

SET_TARGET_PROPERTIES(simulant
                      PROPERTIES
                      VERSION 0.0.1
                      SOVERSION 1)

get_target_property(SONAME simulant SOVERSION)
get_target_property(VERSION simulant VERSION)

FILE(WRITE ${CMAKE_BINARY_DIR}/simulant.soname ${SONAME})
FILE(WRITE ${CMAKE_BINARY_DIR}/simulant.version ${VERSION})

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

SET(ASSET_ROOT ${CMAKE_SOURCE_DIR}/assets)

FILE(GLOB_RECURSE materials "${ASSET_ROOT}/materials/opengl-2.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials/opengl-2.x)

FILE(GLOB_RECURSE materials "${ASSET_ROOT}/materials/opengl-1.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials/opengl-1.x)

FILE(GLOB textures "${ASSET_ROOT}/materials/*.png")
INSTALL(FILES ${textures} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials)

FILE(GLOB_RECURSE materials "${ASSET_ROOT}/materials/*.vert")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials)

FILE(GLOB_RECURSE materials "${ASSET_ROOT}/materials/*.frag")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/materials)

FILE(GLOB particles "${ASSET_ROOT}/particles/*.kglp")
INSTALL(FILES ${particles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/particles)

FILE(GLOB fonts "${ASSET_ROOT}/fonts/orbitron/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant/fonts/orbitron)

INSTALL(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/template" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant)

INSTALL(DIRECTORY "${CMAKE_SOURCE_DIR}/toolchains" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/simulant)


INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/bin/simulant DESTINATION bin)
