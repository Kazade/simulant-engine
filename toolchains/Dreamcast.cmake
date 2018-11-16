SET(CMAKE_SYSTEM_NAME Generic)
SET(DREAMCAST_BUILD TRUE)
SET(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER "$ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "$ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-g++")
set(CMAKE_AR "$ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "$ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_ASM_COMPILER "$ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-as")
set(CMAKE_LINKER "$ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-g++")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_SYSTEM_INCLUDE_PATH "$ENV{KOS_BASE}/include")

SET(CMAKE_EXECUTABLE_SUFFIX ".elf")
SET(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")


INCLUDE_DIRECTORIES(
    $ENV{KOS_BASE}/include
    $ENV{KOS_BASE}/kernel/arch/$ENV{KOS_ARCH}/include
    $ENV{KOS_BASE}/addons/include
    $ENV{KOS_PORTS}/include
)

LINK_DIRECTORIES(
    $ENV{KOS_BASE}/lib/$ENV{KOS_ARCH}
    $ENV{KOS_BASE}/addons/lib/$ENV{KOS_ARCH}
    $ENV{KOS_PORTS}/lib
)

LINK_LIBRARIES(-Wl,--start-group -lkallisti -lc -lgcc -Wl,--end-group)

ADD_DEFINITIONS(
    -D__DREAMCAST__
    -DDREAMCAST
    -D_arch_dreamcast
    -D__arch_dreamcast
    -D_arch_sub_pristine
)

SET(
    CMAKE_C_FLAGS
    ${CMAKE_C_FLAGS}
    "-fno-builtin -fno-strict-aliasing"
)

SET(
    CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS}
    "-fno-builtin -fno-strict-aliasing"
)


SET(CMAKE_ASM_FLAGS "")
SET(CMAKE_ASM_FLAGS_RELEASE "")

link_libraries(m)
