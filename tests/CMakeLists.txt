FILE(GLOB TEST_FILES *.h)
FILE(GLOB TEST_SOURCES global.cpp)
FILE(GLOB LIBGL_CONTAINERS ${CMAKE_SOURCE_DIR}/deps/libgl/containers/*.c)

IF(PLATFORM_DREAMCAST)
FILE(GLOB GL_TESTS gl1/*.h)
ELSEIF(PLATFORM_PSP)
FILE(GLOB GL_TESTS gl1/*.h)
ELSE()
FILE(GLOB GL_TESTS gl2/*.h gl1/*.h)
ENDIF()

# Add the root directory so that we can include from "simulant/X" in tests
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

SET(TEST_GENERATOR_BIN ${CMAKE_SOURCE_DIR}/tools/test_generator.py)
SET(TEST_MAIN_FILENAME main.cpp)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TEST_MAIN_FILENAME}
    COMMAND ${TEST_GENERATOR_BIN} --output ${CMAKE_CURRENT_BINARY_DIR}/${TEST_MAIN_FILENAME} ${TEST_FILES} ${GL_TESTS}
    DEPENDS ${TEST_FILES} ${GL_TESTS} ${TEST_GENERATOR_BIN}
)

EXECUTE_PROCESS(COMMAND ${TEST_GENERATOR_BIN} --list-tests ${TEST_FILES} ${GL_TESTS} OUTPUT_VARIABLE TEST_LIST RESULT_VARIABLE TEST_DISCOVERY_RESULT)

IF(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-access-control")
ENDIF()

IF(PLATFORM_PSP)
    SET(PSP_TITLE "Simulant Tests")
    SET(PSP_MAJOR_VERSION 1)
    SET(PSP_MINOR_VERSION 0)
    configure_file(${CMAKE_SOURCE_DIR}/conf/psp_module_info.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info.cpp)
    set(TEST_SOURCES ${TEST_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info.cpp)
endif()

IF(ANDROID_API)
    set(TEST_SOURCES ${TEST_SOURCES} ${CMAKE_SOURCE_DIR}/platforms/android/SDL_android_main.c)
ENDIF()

ADD_EXECUTABLE(simulant_tests ${TEST_FILES} ${TEST_SOURCES} ${LIBGL_CONTAINERS} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_MAIN_FILENAME}
    test_gltf.h)

IF(${TEST_DISCOVERY_RESULT} EQUAL 0)
    STRING(REPLACE "\n" ";" TEST_LIST ${TEST_LIST})
    FOREACH(test_name ${TEST_LIST})
        STRING(STRIP "${test_name}" test_name)
        ADD_TEST(NAME "${test_name}" COMMAND simulant_tests ${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    ENDFOREACH()
ENDIF()

IF(PLATFORM_PSP)
    create_pbp_file(TARGET simulant_tests TITLE "Simulant Tests")

    add_custom_command(
        TARGET simulant_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ../../assets
        ${CMAKE_CURRENT_BINARY_DIR}/simulant
    )

    add_custom_command(
        TARGET simulant_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ../../samples/data
        ${CMAKE_CURRENT_BINARY_DIR}/sample_data
    )
ENDIF()

if(PLATFORM_XBOX)
    # Convert EXE to XBE
    add_custom_target(simulant_tests_cxbe_convert ALL
        COMMENT "CXBE Conversion: [EXE -> XBE]"
        VERBATIM
        COMMAND "${CMAKE_COMMAND}" -E env
                ${NXDK_DIR}/tools/cxbe/cxbe
                -OUT:${CMAKE_CURRENT_BINARY_DIR}/simulant_tests.xbe
                -TITLE:simulant_tests
                ${CMAKE_CURRENT_BINARY_DIR}/simulant_tests.exe
    )

    add_dependencies(simulant_tests_cxbe_convert simulant_tests)

    set(XBOX_ISO_DIR "${CMAKE_CURRENT_BINARY_DIR}/simulant_tests.dir/")
    file(MAKE_DIRECTORY ${XBOX_ISO_DIR})
    # Convert XBE to XISO
    add_custom_target(simulant_tests_xbe_iso ALL
        COMMENT "XISO Conversion: [XBE -> XISO]"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_BINARY_DIR}/simulant_tests.xbe"
                "${XBOX_ISO_DIR}/default.xbe"
        COMMAND "${CMAKE_COMMAND}" -E env
                ${NXDK_DIR}/tools/extract-xiso/build/extract-xiso
                -c ${CMAKE_CURRENT_BINARY_DIR}
                ${XBOX_ISO_DIR}/simulant_tests.iso
        WORKING_DIRECTORY ${XBOX_ISO_DIR}
        VERBATIM
    )

    add_dependencies(simulant_tests_xbe_iso simulant_tests_cxbe_convert)
endif()

IF(WIN32 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
# Necessary when compiling tests on mingw because the main.cpp file gets too large
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wa,-mbig-obj")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
ENDIF()

TARGET_LINK_LIBRARIES(
    simulant_tests
    simulant
)
