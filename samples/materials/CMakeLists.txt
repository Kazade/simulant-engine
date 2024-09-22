cmake_minimum_required(VERSION 3.9)
project(Materials)

OPTION(BUILD_TESTS "Enable test compilation" OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT FLTO_SUPPORTED OUTPUT FLTO_ERROR)

SET(OpenGL_GL_PREFERENCE "LEGACY")

IF(SIMULANT_FLAVOUR)
    MESSAGE("-- Building Flavour: ${SIMULANT_FLAVOUR}")
ENDIF()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tools/simulant/cmake_modules")

FIND_PACKAGE(PkgConfig REQUIRED)

IF(ANDROID)
    FIND_PACKAGE(simulant REQUIRED)
    LINK_LIBRARIES(simulant::simulant)
ELSE()
    FIND_PACKAGE(Simulant REQUIRED)
    INCLUDE_DIRECTORIES(BEFORE ${SIMULANT_INCLUDE_DIR})
    LINK_LIBRARIES(${SIMULANT_LIBRARY})
ENDIF()

IF(PLATFORM_DREAMCAST)
    LINK_LIBRARIES(stdc++ kallisti gcc c kallisti ppp m)
ELSEIF(PLATFORM_PSP)
    ADD_DEFINITIONS(-D_PSP_FW_VERSION=150)
ELSEIF(ANDROID)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    SET(GL_LIBRARIES GLESv2 EGL)
    FIND_PATH(GL_INCLUDE_DIRS NAMES gl2.h PATH_SUFFIXES GLES2)
    INCLUDE_DIRECTORIES(${GL_INCLUDE_DIRS})
    LINK_LIBRARIES(${GL_LIBRARIES} m)
ELSE()
    FIND_PACKAGE(SDL2 REQUIRED)
    FIND_PACKAGE(OpenAL REQUIRED)
    FIND_PACKAGE(OpenGL REQUIRED)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${GL_INCLUDE_DIRS})
    LINK_LIBRARIES(${SDL2_LIBRARIES} ${GL_LIBRARIES} m)
ENDIF()

IF(WIN32)
    IF(MINGW)
        # mingw has bugs which prevent flto
        SET(FLTO_SUPPORTED FALSE)
    endif()

    add_definitions(-D_WIN32_WINNT=0x0601 -D_WINSOCK_DEPRECATED_NO_WARNINGS -Dmain=SDL_main)
    link_libraries(wsock32 ws2_32 iphlpapi)
ENDIF()

FILE(GLOB_RECURSE SOURCES FOLLOW_SYMLINKS ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.c)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

IF(PLATFORM_DREAMCAST)
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mfsrra -mfsca -ffast-math")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -mfsrra -mfsca -ffast-math")
ENDIF()

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0 -s -fomit-frame-pointer -fstrict-aliasing")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -fstrict-aliasing")

# This should happen automatically, but, it doesn't seem to on the Dreamcast
if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
    add_definitions(-DKOS_DEBUG=0)
endif()

IF(PLATFORM_PSP)
    SET(PSP_TITLE "materials")
    SET(PSP_MAJOR_VERSION 1)
    SET(PSP_MINOR_VERSION 0)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf/psp_module_info.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info_materials.cpp)
    set(SAMPLE_SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info_materials.cpp)
endif()

add_definitions("-Wall")

IF(ANDROID)
    add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    add_library(materials SHARED ${SOURCES})
    target_link_libraries(materials android native_app_glue log)
ELSE()
    add_executable(materials ${SOURCES})
ENDIF()

if(FLTO_SUPPORTED)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET materials PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${FLTO_ERROR}>")
endif()


IF(PLATFORM_PSP)
    message("Creating PBP")
    create_pbp_file(TARGET materials)
ENDIF()

# On the Dreamcast it's important to move debugging symbols
# from the .elf to a separate file otherwise the generated .elf
# will be too large for the amount of RAM
if(PLATFORM_DREAMCAST)
if(CMAKE_BUILD_TYPE MATCHES Debug)

set(EXE_FILENAME materials.elf)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME} ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug && ${CMAKE_OBJCOPY} -S ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}
    DEPENDS materials
    COMMENT "Separating debug info for ${EXE_FILENAME}"
)
add_custom_target(${EXE_FILENAME}.debug.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug)

endif()
endif()

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)

IF(BUILD_TESTS)
    ## Add the test executable
    ENABLE_TESTING()

    # This is the path to the Simulant test generator
    SET(TEST_GENERATOR ${CMAKE_SOURCE_DIR}/tools/simulant/test_generator.py)

    # This is the path to the generate test main.cpp
    SET(TEST_MAIN_FILENAME /tests/main.cpp)

    # We include all project source files, except main.cpp so we can link in the tests
    SET(TEST_SOURCES ${SOURCES})
    LIST(REMOVE_ITEM TEST_SOURCES ${CMAKE_SOURCE_DIR}/sources/main.cpp)

    # Find all the tests, these are header files under "tests"
    FILE(GLOB TEST_FILES ${CMAKE_SOURCE_DIR}/tests/*.h)

    IF(TEST_FILES)
        # Generate the test main.cpp
        ADD_CUSTOM_COMMAND(
            OUTPUT ${CMAKE_SOURCE_DIR}${TEST_MAIN_FILENAME}
            COMMAND ${TEST_GENERATOR} --output ${CMAKE_SOURCE_DIR}${TEST_MAIN_FILENAME} ${TEST_FILES}
            DEPENDS ${TEST_FILES} ${TEST_GENERATOR}
        )

        # Add the tests executable
        ADD_EXECUTABLE(
            tests
            tests/main.cpp
            ${TEST_SOURCES}
        )

        ADD_TEST(tests tests)
    ENDIF()
ENDIF(BUILD_TESTS)

# Installation rules!
FILE(GLOB ASSET_FILES ${CMAKE_SOURCE_DIR}/assets/.*)
INSTALL(TARGETS materials RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT RUNTIME)
INSTALL(FILES ${ASSET_FILES} DESTINATION "share/materials/assets")
