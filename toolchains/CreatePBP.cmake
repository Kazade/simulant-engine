# File defining macro outputting PSP-specific EBOOT.PBP out of passed executable target.
# Copyright 2020 - Daniel 'dbeef' Zalega
#
# Args:
# TARGET - defined by an add_executable call before calling create_pbp_file
# TITLE - optional, string, target's name in PSP menu
# ICON_PATH - optional, absolute path to .png file, 144x82
# BACKGROUND_PATH - optional, absolute path to .png file, 480x272
# PREVIEW_PATH - optional, absolute path to .png file, 480x272
# CREATE_PRX - optional, create a .prx file from the .elf (default: true)
#
cmake_minimum_required(VERSION 3.10)

macro(create_pbp_file)

    set(oneValueArgs TARGET TITLE ICON_PATH BACKGROUND_PATH PREVIEW_PATH CREATE_PRX)
    cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # As pack-pbp takes undefined arguments in form of "NULL" string,
    # set each undefined macro variable to such value:
    foreach(arg ${oneValueArgs})
        if (NOT DEFINED ARG_${arg})
            if(${arg} STREQUAL "CREATE_PRX")
                set(ARG_CREATE_PRX TRUE)
            else()
                set(ARG_${arg} "NULL")
            endif()
        endif()
    endforeach()

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        if(NOT ${ARG_CREATE_PRX})
            add_custom_command(
                TARGET ${ARG_TARGET}
                POST_BUILD COMMAND
                "${STRIP}" "$<TARGET_FILE:${ARG_TARGET}>"
                COMMENT "Stripping binary"
            )
        endif()
    else()
        add_custom_command(
            TARGET ${ARG_TARGET}
            POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Not stripping binary, build type is ${CMAKE_BUILD_TYPE}."
        )
    endif()

    add_custom_command(
            TARGET ${ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact
            COMMENT "Creating psp_artifact directory."
    )

    add_custom_command(
            TARGET ${ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${ARG_TARGET}>
            $<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact
            COMMENT "Copying ELF to psp_artifact directory."
    )

    add_custom_command(
            TARGET ${ARG_TARGET}
            POST_BUILD COMMAND
            "${FIXUP}" "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/${ARG_TARGET}"
            COMMENT "Calling psp-fixup-imports"
    )

    if(${ARG_CREATE_PRX})
        set(TARGET_FILENAME "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/${ARG_TARGET}.prx")
        add_custom_command(
                TARGET ${ARG_TARGET}
                POST_BUILD COMMAND
                "${PRXGEN}" "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/${ARG_TARGET}" "${TARGET_FILENAME}"
                COMMENT "Calling psp-prxgen"
        )
    else()
        set(TARGET_FILENAME "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/${ARG_TARGET}")
    endif()

    add_custom_command(
            TARGET ${ARG_TARGET}
            POST_BUILD COMMAND
            "${MKSFOEX}" "-d" "MEMSIZE=1" "${ARG_TITLE}" "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/PARAM.SFO"
            COMMENT "Calling mksfoex"
    )

    add_custom_command(
            TARGET ${ARG_TARGET}
            POST_BUILD COMMAND
            "${PACK_PBP}" "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/EBOOT.PBP" "$<TARGET_FILE_DIR:${ARG_TARGET}>/psp_artifact/PARAM.SFO" "${ARG_ICON_PATH}" "NULL" "${ARG_PREVIEW_PATH}"
            "${ARG_BACKGROUND_PATH}" "NULL" "${TARGET_FILENAME}" "NULL"
            COMMENT "Calling pack-pbp"
    )

    add_custom_command(
            TARGET ${ARG_TARGET}
            POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E cmake_echo_color --cyan "EBOOT.PBP file created."
    )

endmacro()
