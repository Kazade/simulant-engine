FILE(GLOB TEST_FILES *.h)
FILE(GLOB TEST_SOURCES global.cpp)
FILE(GLOB LIBGL_CONTAINERS ${CMAKE_SOURCE_DIR}/deps/libgl/containers/*.c)

IF(DREAMCAST_BUILD)
FILE(GLOB GL_TESTS gl1/*.h)
ELSE()
FILE(GLOB GL_TESTS gl2/*.h gl1/*.h)
ENDIF()

# Add the root directory so that we can include from "simulant/X" in tests
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

SET(TEST_GENERATOR_BIN ${CMAKE_SOURCE_DIR}/tools/test_generator.py)

IF(DREAMCAST_BUILD)
SET(TEST_MAIN_FILENAME main-dc.cpp)
ELSE()
SET(TEST_MAIN_FILENAME main.cpp)
ENDIF()

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME}
    COMMAND ${TEST_GENERATOR_BIN} --output ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME} ${TEST_FILES} ${GL_TESTS}
    DEPENDS ${TEST_FILES} ${GL_TESTS} ${TEST_GENERATOR_BIN}
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-access-control")

IF(ANDROID)
    # If we're building for Android, we need to link with SDL_android_main.c which should
    # exist in the current directory. You may need to copy it there after running the prepare
    # script
    SET(TEST_SOURCES ${TEST_SOURCES} ./SDL_android_main.c)
    MESSAGE("-- Android platforms, so added SDL_android_main.c")
    MESSAGE("-- Sources: ${TEST_SOURCES}")
ENDIF()

if(ANDROID)
    # On Android we build a .so and then use gradle to build the test application
    ADD_LIBRARY(
        simulant_tests
        SHARED
        ${TEST_FILES} ${TEST_SOURCES} ${LIBGL_CONTAINERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME}
    )
else()
    ADD_EXECUTABLE(simulant_tests ${TEST_FILES} ${TEST_SOURCES} ${LIBGL_CONTAINERS} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN_FILENAME})
    ADD_TEST(simulant_suite simulant_tests)
endif()

IF(WIN32)
# Necessary when compiling tests on mingw because the main.cpp file gets too large
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wa,-mbig-obj")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
ENDIF()


TARGET_LINK_LIBRARIES(
    simulant_tests
    simulant
)

