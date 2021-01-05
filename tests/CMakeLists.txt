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

IF(PLATFORM_DREAMCAST)
SET(TEST_MAIN_FILENAME main-dc.cpp)
ELSEIF(PLATFORM_PSP)
SET(TEST_MAIN_FILENAME main-psp.cpp)
ELSE()
SET(TEST_MAIN_FILENAME main.cpp)
ENDIF()

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME}
    COMMAND ${TEST_GENERATOR_BIN} --output ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME} ${TEST_FILES} ${GL_TESTS}
    DEPENDS ${TEST_FILES} ${GL_TESTS} ${TEST_GENERATOR_BIN}
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-access-control")

IF(PLATFORM_PSP)
    SET(PSP_TITLE "Simulant Tests")
    SET(PSP_MAJOR_VERSION 1)
    SET(PSP_MINOR_VERSION 0)
    configure_file(${CMAKE_SOURCE_DIR}/conf/psp_module_info.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info.cpp)
    set(TEST_SOURCES ${TEST_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info.cpp)
endif()

ADD_EXECUTABLE(simulant_tests ${TEST_FILES} ${TEST_SOURCES} ${LIBGL_CONTAINERS} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME})
ADD_TEST(simulant_suite simulant_tests)

IF(PLATFORM_PSP)
    create_pbp_file(TARGET simulant_tests)
ENDIF()

IF(WIN32)
# Necessary when compiling tests on mingw because the main.cpp file gets too large
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wa,-mbig-obj")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
ENDIF()

TARGET_LINK_LIBRARIES(
    simulant_tests
    simulant
)

