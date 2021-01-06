
LINK_LIBRARIES(
    simulant
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(sample sample.cpp)
ADD_EXECUTABLE(2d_sample 2d_sample.cpp)
ADD_EXECUTABLE(viewport_sample viewport_sample.cpp)
ADD_EXECUTABLE(selection_sample selection_sample.cpp)
ADD_EXECUTABLE(light_sample light_sample.cpp)
ADD_EXECUTABLE(rtt_sample rtt_sample.cpp)
ADD_EXECUTABLE(q2bsp_sample q2bsp_sample.cpp)
ADD_EXECUTABLE(fleets_demo fleets_demo.cpp)
ADD_EXECUTABLE(terrain_demo terrain_sample.cpp)
ADD_EXECUTABLE(physics_demo physics_demo.cpp)
ADD_EXECUTABLE(nehe02 nehe02.cpp)
ADD_EXECUTABLE(nehe03 nehe03.cpp)
ADD_EXECUTABLE(ui_demo ui_demo.cpp)
ADD_EXECUTABLE(particles particles.cpp)


SET(SAMPLES
    sample
    2d_sample
    viewport_sample
    selection_sample
    light_sample
    rtt_sample
    q2bsp_sample
    fleets_demo
    terrain_demo
    physics_demo
    nehe02
    nehe03
    ui_demo
    particles
)

SET(EXE_EXTENSION ".elf")
IF(WIN32)
    SET(EXE_EXTENSION ".exe")
ENDIF()


foreach(sample ${SAMPLES})
    SET(EXE_FILENAME ${sample}${EXE_EXTENSION})

    if(SIMULANT_SEPERATE_DEBUGINFO)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug
            COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME} ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug && ${CMAKE_OBJCOPY} -S ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}
            DEPENDS ${sample}
            COMMENT "Separating debug info for ${EXE_FILENAME}"
        )
        add_custom_target(${EXE_FILENAME}.debug.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug)
    endif()

    IF(PLATFORM_DREAMCAST)
    IF(SIMULANT_BUILD_SAMPLE_CDI)
        SET(EXE ${sample})
        ADD_CUSTOM_TARGET(
            ${EXE}.cdi ALL
            COMMAND $ENV{KOS_OBJCOPY} -R .stack -O binary -S -g ${CMAKE_CURRENT_BINARY_DIR}/${EXE}.elf ${CMAKE_CURRENT_BINARY_DIR}/${EXE}.bin
            COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/${EXE}
            COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${EXE}
            COMMAND $ENV{KOS_BASE}/utils/scramble/scramble ${CMAKE_CURRENT_BINARY_DIR}/${EXE}.bin ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/1ST_READ.BIN
            COMMAND IP_TEMPLATE_FILE=${CMAKE_SOURCE_DIR}/platforms/dreamcast/IP.TMPL $ENV{KOS_BASE}/utils/makeip/makeip ${CMAKE_SOURCE_DIR}/platforms/dreamcast/ip.txt ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/IP.BIN
            COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/fonts ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/materials ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/particles ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/textures ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/assets/sounds ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/simulant
            COMMAND cp -r ${CMAKE_SOURCE_DIR}/samples/data ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/sample_data
            COMMAND mkisofs -C 0,11702 -V ${EXE_FILENAME} -G ${CMAKE_CURRENT_BINARY_DIR}/${EXE}/IP.BIN -joliet -rock -l -o ${EXE}.iso ${CMAKE_CURRENT_BINARY_DIR}/${EXE}
            COMMAND $ENV{KOS_BASE}/utils/img4dc/build/cdi4dc/cdi4dc ${CMAKE_CURRENT_BINARY_DIR}/${EXE}.iso ${CMAKE_CURRENT_BINARY_DIR}/${EXE}.cdi
            DEPENDS ${EXE_FILENAME}.debug.target
            COMMENT "Converting ${EXE}.elf to ${EXE}.cdi"
        )
    endif()
    endif()
endforeach()

