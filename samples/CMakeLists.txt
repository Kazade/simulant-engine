
LINK_LIBRARIES(
    simulant
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

SET(SAMPLES
    sample
    viewport_sample
    selection_sample
    light_sample
    rtt_sample
    q2bsp_sample
    fleets_demo
    terrain_sample
    physics_demo
    nehe02
    nehe03
    ui_demo
    particles
    cave_demo
)

SET(EXE_EXTENSION ".elf")
IF(WIN32)
    SET(EXE_EXTENSION ".exe")
ELSEIF(PLATFORM_PSP)
    SET(EXE_EXTENSION "")
ENDIF()


foreach(sample ${SAMPLES})
    SET(SAMPLE_SOURCES ${sample}.cpp)

    IF(PLATFORM_PSP)
        SET(PSP_TITLE "${sample}")
        SET(PSP_MAJOR_VERSION 1)
        SET(PSP_MINOR_VERSION 0)
        configure_file(${CMAKE_SOURCE_DIR}/conf/psp_module_info.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info_${sample}.cpp)
        set(SAMPLE_SOURCES ${SAMPLE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info_${sample}.cpp)
    endif()

    ADD_EXECUTABLE(${sample} ${SAMPLE_SOURCES})

    IF(PLATFORM_PSP)
        create_pbp_file(TARGET ${sample})
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${sample}_psp)
        add_custom_command(
            TARGET ${sample} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/psp_artifact/EBOOT.PBP
            ${CMAKE_CURRENT_BINARY_DIR}/${sample}_psp/EBOOT.PBP
        )
        add_custom_command(
            TARGET ${sample} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ../../../assets
            ${CMAKE_CURRENT_BINARY_DIR}/${sample}_psp/simulant
        )

        add_custom_command(
            TARGET ${sample} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ../../../samples/data
            ${CMAKE_CURRENT_BINARY_DIR}/${sample}_psp/sample_data
        )
    ENDIF()

    SET(EXE_FILENAME ${sample}${EXE_EXTENSION})

    if(NOT PLATFORM_PSP)
        if(SIMULANT_SEPERATE_DEBUGINFO)
            add_custom_command(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug
                COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME} ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug && ${CMAKE_OBJCOPY} -S ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}
                DEPENDS ${sample}
                COMMENT "Separating debug info for ${EXE_FILENAME}"
            )
            add_custom_target(${EXE_FILENAME}.debug.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug)
        endif()
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

