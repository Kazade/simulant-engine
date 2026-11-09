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

# Copy Lua test scripts from tests/data/ to tests/data/ in the build output
# directory so tests can read them instead of writing to temp directories
# (Dreamcast-safe). The tests open them as "tests/data/<name>.lua", so this
# keeps the same relative layout whether they run from the source root
# (ctest) or from the build root (CI).
FILE(GLOB LUA_TEST_SCRIPTS ${CMAKE_SOURCE_DIR}/tests/data/*.lua)
FOREACH(script ${LUA_TEST_SCRIPTS})
  GET_FILENAME_COMPONENT(filename ${script} NAME)
  ADD_CUSTOM_COMMAND(
        TARGET simulant_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${script}
            ${CMAKE_CURRENT_BINARY_DIR}/data/${filename}
    )
ENDFOREACH()

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

IF(WIN32 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # Necessary when compiling tests on mingw because the main.cpp file gets too large
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wa,-mbig-obj")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
ENDIF()

TARGET_LINK_LIBRARIES(
    simulant_tests
    simulant
)

IF(IOS)
  # On iOS the tests run as an app: UIKit has to own the main thread, so the
  # generated main() hands over to smlt::ios_main(). It's put together here
  # as SimulantTests.app: the executable, an Info.plist, and the assets and
  # test data that the tests read relative to the working directory (which
  # ios_main() makes the bundle). platforms/ios/run_simulator_tests.sh runs
  # it in the iOS simulator.
  SET_TARGET_PROPERTIES(simulant_tests PROPERTIES MACOSX_BUNDLE OFF)

  SET(IOS_EXECUTABLE_NAME simulant_tests)
  SET(IOS_BUNDLE_IDENTIFIER org.simulant.tests)
  SET(IOS_BUNDLE_NAME SimulantTests)
  SET(IOS_BUNDLE_VERSION 1.0)
  IF(IOS_SIMULATOR)
    SET(IOS_SUPPORTED_PLATFORM iPhoneSimulator)
  ELSE()
    SET(IOS_SUPPORTED_PLATFORM iPhoneOS)
  ENDIF()
  CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/platforms/ios/Info.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    @ONLY
  )

  SET(TEST_APP ${CMAKE_CURRENT_BINARY_DIR}/SimulantTests.app)
  ADD_CUSTOM_COMMAND(
    TARGET simulant_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${TEST_APP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_APP}/tests
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:simulant_tests> ${TEST_APP}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/Info.plist ${TEST_APP}/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/assets ${TEST_APP}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/data ${TEST_APP}/tests/data
    COMMENT "Putting SimulantTests.app together"
  )
ENDIF()
