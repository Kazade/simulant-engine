SET(CMAKE_SYSTEM_NAME Generic)
SET(DREAMCAST_BUILD TRUE)
SET(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER "kos-cc")
set(CMAKE_CXX_COMPILER "kos-c++")
set(CMAKE_AR "kos-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "kos-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_ASM_COMPILER "kos-as")
set(CMAKE_LINKER "kos-ld")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_SYSTEM_INCLUDE_PATH "$ENV{KOS_BASE}/include")

SET(CMAKE_EXECUTABLE_SUFFIX ".elf")
SET(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

add_definitions("-DDREAMCAST")
add_definitions("-D_arch_dreamcast")
add_definitions("-D_arch_sub_pristine")
