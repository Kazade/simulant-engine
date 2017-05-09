SET(CMAKE_SYSTEM_NAME Generic)
SET(DREAMCAST_BUILD TRUE)
SET(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CROSSCOMPILING TRUE)


if ("${CMAKE_C_COMPILER}" STREQUAL "")
    set(CMAKE_C_COMPILER "kos-cc")
endif()

if ("${CMAKE_CXX_COMPILER}" STREQUAL "")
    set(CMAKE_CXX_COMPILER "kos-c++")
endif()

if ("${CMAKE_AR}" STREQUAL "")
    set(CMAKE_AR "kos-ar")
endif()

if ("${CMAKE_RANLIB}" STREQUAL "")
    set(CMAKE_RANLIB "kos-ranlib")
endif()


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_SYSTEM_INCLUDE_PATH "$ENV{KOS_BASE}/include")

SET(CMAKE_EXECUTABLE_SUFFIX ".elf")
SET(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

add_definitions("-DDREAMCAST")
add_definitions("-D_arch_dreamcast")
add_definitions("-D_arch_sub_pristine")
