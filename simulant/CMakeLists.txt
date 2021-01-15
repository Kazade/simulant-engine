FILE(GLOB_RECURSE SIMULANT_FILES FOLLOW_SYMLINKS *.cpp *.c *.cc)

# Remove all the renderer files so we can add them specifically
FILE(GLOB_RECURSE TO_REMOVE
    "platforms/*"  # Remove platform specific additions
    "renderers/glad/*"
    "renderers/gl2x/*"
    "deps/SOIL/original/*"
    "deps/bounce_src/bounce/garment/triangle/tricall.c"
    "sdl2_window.*"
    "kos_window.*"
    "sound_drivers/*"
)

LIST(REMOVE_ITEM SIMULANT_FILES ${TO_REMOVE})

# Include GL2x stuff on non-DC builds
IF(NOT PLATFORM_DREAMCAST)
    IF(NOT PLATFORM_PSP)
        MESSAGE("-- Including GLAD in build")
        # DC build doesn't need glad, GL1 is available directly
        FILE(GLOB_RECURSE GLAD1_FILES renderers/glad/*.c)
        SET(SIMULANT_FILES ${SIMULANT_FILES} ${GLAD1_FILES})

        MESSAGE("-- Including GL2X in build")
        FILE(GLOB_RECURSE GL2_FILES renderers/gl2x/*.c*)
        SET(SIMULANT_FILES ${SIMULANT_FILES} ${GL2_FILES})
    ENDIF()
ENDIF()

IF(PLATFORM_PSP)
    MESSAGE("-- Including pspgl in build as this is the PSP")
    FILE(GLOB LIBGL ${CMAKE_SOURCE_DIR}/deps/pspgl/src/*.c)

    # These are dead files?
    LIST(
        REMOVE_ITEM
        LIBGL
        ${CMAKE_SOURCE_DIR}/deps/pspgl/src/glGenLists.c
        ${CMAKE_SOURCE_DIR}/deps/pspgl/src/glIsList.c
        ${CMAKE_SOURCE_DIR}/deps/pspgl/src/glDeleteLists.c
    )

    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/pspgl/include)
    SET(SIMULANT_FILES ${SIMULANT_FILES} ${LIBGL})

    SET(PROC_FILE ${CMAKE_SOURCE_DIR}/deps/pspgl/include/pspgl_proctable.h)
    add_custom_command(
        OUTPUT ${PROC_FILE}
        COMMAND ${CMAKE_SOURCE_DIR}/conf/extract_gl_extensions.py
        ${PROC_FILE}
        ${LIBGL}
        COMMENT "Generating pspgl extension list"
    )

    SET(SIMULANT_FILES ${SIMULANT_FILES} ${PROC_FILE})
ENDIF()

IF(PLATFORM_DREAMCAST)
    MESSAGE("-- Including GLdc and ALdc in build, as this is the Dreamcast")

    # We build in GLdc and ALdc directly (as it's all under the Simulant umbrella)
    FILE(GLOB_RECURSE LIBGL ${CMAKE_SOURCE_DIR}/deps/libgl/*.c)
    FILE(GLOB_RECURSE LIBGL_SAMPLES ${CMAKE_SOURCE_DIR}/deps/libgl/samples/*.c)
    LIST(REMOVE_ITEM LIBGL ${LIBGL_SAMPLES})

    FILE(GLOB_RECURSE PLATFORM ${CMAKE_CURRENT_SOURCE_DIR}/platforms/dreamcast/*.c)

    IF(PLATFORM)
        MESSAGE("-- Including platform specific files: ${PLATFORM}")
    ENDIF()

    SET(
        LIBAL
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/buffer.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/core.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/context.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/alut.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/math.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/listener.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/platform.c
        ${CMAKE_SOURCE_DIR}/deps/libAL/src/source.c
    )

    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/libgl/include ${CMAKE_SOURCE_DIR}/deps/libAL/include)

    SET(SIMULANT_FILES ${SIMULANT_FILES}
        ${LIBGL}
        ${LIBGL_ASM}
        ${LIBAL}
        ${PLATFORM}
    )

    SET(
        SIMULANT_FILES
        ${SIMULANT_FILES}
        kos_window.cpp
        sound_drivers/openal_sound_driver.cpp
        sound_drivers/al_error.cpp
        sound_drivers/null_sound_driver.cpp
    )
ELSEIF(PLATFORM_PSP)
    FILE(GLOB_RECURSE PLATFORM ${CMAKE_CURRENT_SOURCE_DIR}/platforms/psp/*.cpp)

    IF(PLATFORM)
        MESSAGE("-- Including platform specific files: ${PLATFORM}")
    ENDIF()

    SET(
        SIMULANT_FILES
        ${SIMULANT_FILES}
        ${PLATFORM}
        # FIXME: Use OpenAL on PSP
        # sound_drivers/openal_sound_driver.cpp
        # sound_drivers/al_error.cpp
        sound_drivers/null_sound_driver.cpp
    )
ELSE()
    SET(
        SIMULANT_FILES
        ${SIMULANT_FILES}
        sdl2_window.cpp
        sound_drivers/openal_sound_driver.cpp
        sound_drivers/al_error.cpp
        sound_drivers/null_sound_driver.cpp
    )
ENDIF()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/bounce/include)

IF(PLATFORM_DREAMCAST OR PLATFORM_PSP)
ADD_LIBRARY(simulant STATIC ${SIMULANT_FILES})
ELSE()
ADD_LIBRARY(simulant SHARED ${SIMULANT_FILES})
ENDIF()

IF(MINGW)

    FIND_LIBRARY(OPENAL_LIB NAMES OpenAL32.lib PATHS $ENV{OPENALDIR}/libs/Win64)
    message(${OPENAL_LIB})

    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ ")

    IF(OPENAL_LIB)
        TARGET_LINK_LIBRARIES(simulant LINK_PUBLIC opengl32 glu32 mingw32 psapi $ENV{SDL2DIR}/lib/x64/SDL2.lib $ENV{OPENALDIR}/libs/Win64/OpenAL32.lib $ENV{ZLIBDIR}/lib_release/zlib.lib userenv)
    ELSE()
        SET(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ ")
        SET(CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_MODULE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ ")
        TARGET_LINK_LIBRARIES(simulant LINK_PUBLIC opengl32 glu32 mingw32 psapi SDL2main SDL2 OpenAL32 userenv)
    ENDIF()
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
