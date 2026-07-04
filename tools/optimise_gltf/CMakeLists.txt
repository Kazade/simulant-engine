cmake_minimum_required(VERSION 3.16)
project(optimise_gltf CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(optimise_gltf optimise_gltf.cpp)

if(NOT MSVC)
    target_compile_options(optimise_gltf PRIVATE -Wall -Wextra)
endif()

# Location of the texconv tool. Can be overridden with -DTEXCONV=/path/to/texconv
find_program(TEXCONV texconv)
if(TEXCONV)
    target_compile_definitions(optimise_gltf PRIVATE DEFAULT_TEXCONV="${TEXCONV}")
endif()

install(TARGETS optimise_gltf RUNTIME DESTINATION bin)
