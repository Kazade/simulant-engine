set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
SET(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc-11)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++-11)
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)

SET(CMAKE_INCLUDE_PATH "/usr/include")
SET(CMAKE_LIBRARY_PATH "/usr/lib/arm-linux-gnueabihf")
SET(CMAKE_PROGRAM_PATH "/usr/bin/arm-linux-gnueabihf")

set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/arm-linux-gnueabihf/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "/usr/lib/pkgconfig:/usr/share/pkgconfig")

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

SET(CMAKE_CROSSCOMPILING_EMULATOR "qemu-arm;-L;/usr/arm-linux-gnueabihf")

set(CMAKE_C_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=vfpv4")
set(CMAKE_CXX_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=vfpv4")

# cache flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")

set(PLATFORM_EVERCADE TRUE)
add_definitions(-D__EVERCADE__)

