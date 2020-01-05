SET(CMAKE_SYSTEM_NAME Generic)
SET(DREAMCAST_BUILD TRUE)
SET(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER $ENV{KOS_CC_BASE}/bin/sh-elf-gcc)
set(CMAKE_CXX_COMPILER $ENV{KOS_CC_BASE}/bin/sh-elf-g++)

add_compile_options(-ml -m4-single-only -ffunction-sections -fdata-sections)

set(ASMFLAGS "${ASMFLAGS} -little")

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_SYSTEM_INCLUDE_PATH "${CMAKE_SYSTEM_INCLUDE_PATH} $ENV{KOS_BASE}/include $ENV{KOS_BASE}/kernel/arch/dreamcast/include $ENV{KOS_BASE}/addons/include $ENV{KOS_BASE}/../kos-ports/include")

INCLUDE_DIRECTORIES(
    $ENV{KOS_BASE}/include
    $ENV{KOS_BASE}/kernel/arch/dreamcast/include
    $ENV{KOS_BASE}/addons/include
    $ENV{KOS_BASE}/../kos-ports/include
)

LINK_DIRECTORIES(
    $ENV{KOS_BASE}/lib/dreamcast
    $ENV{KOS_BASE}/addons/lib/dreamcast
    $ENV{KOS_PORTS}/lib
)

LINK_LIBRARIES(
    -T$ENV{KOS_BASE}/utils/ldscripts/shlelf.xc
    -nodefaultlibs
    -lstdc++
    -Wl,--start-group -lkallisti -lc -lgcc -Wl,--end-group -ml -m4-single-only -Wl,-Ttext=0x8c010000 -Wl,--gc-sections
)

SET(CMAKE_EXECUTABLE_SUFFIX ".elf")
SET(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

ADD_DEFINITIONS(
    -D__DREAMCAST__
    -DDREAMCAST
    -D_arch_dreamcast
    -D__arch_dreamcast
    -D_arch_sub_pristine
)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    ADD_DEFINITIONS(-DNDEBUG)
endif()

SET(CMAKE_ASM_FLAGS "")
SET(CMAKE_ASM_FLAGS_RELEASE "")

link_libraries(m)

