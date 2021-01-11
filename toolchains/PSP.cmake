#
# CMake toolchain file for PSP.
#
# Copyright 2019 - Wally
# Copyright 2020 - Daniel 'dbeef' Zalega

if (DEFINED PSPDEV)
    # Custom PSPDEV passed as cmake call argument.
else()
    # Determine PSP toolchain installation directory;
    # psp-config binary is guaranteed to be in path after successful installation:
    execute_process(COMMAND bash -c "psp-config --pspdev-path" OUTPUT_VARIABLE PSPDEV OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# Assert that PSP SDK path is now defined:
if (NOT DEFINED PSPDEV)
    message(FATAL_ERROR "PSPDEV not defined. Make sure psp-config in your path or pass custom \
                        toolchain location via PSPDEV variable in cmake call.")
endif ()

# Set helper variables:
set(PSPSDK ${PSPDEV}/psp/sdk)
set(PSPBIN ${PSPDEV}/bin)
set(PSPCMAKE ${PSPDEV}/psp/share/cmake)

# Basic CMake Declarations
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER ${PSPBIN}/psp-gcc)
set(CMAKE_CXX_COMPILER ${PSPBIN}/psp-g++)
set(CMAKE_FIND_ROOT_PATH "${PSPSDK};${PSPDEV}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set paths to PSP-specific utilities:
set(MKSFO ${PSPBIN}/mksfo)
set(MKSFOEX ${PSPBIN}/mksfoex)
set(PACK_PBP ${PSPBIN}/pack-pbp)
set(FIXUP ${PSPBIN}/psp-fixup-imports)
set(PRXGEN ${PSPBIN}/psp-prxgen)
set(ENC ${PSPBIN}/PrxEncrypter)
set(STRIP ${PSPBIN}/psp-strip)

# Include directories:
include_directories(${include_directories} ${PSPDEV}/include ${PSPSDK}/include)

add_definitions("-G0")

# Definitions that may be needed to use some libraries:
add_definitions("-D__PSP__")
add_definitions("-DHAVE_OPENGL")
add_definitions("-DPSP_FW_VERSION=500")

link_directories(
    ${PSPSDK}/lib
    ${PSPDEV}/lib
)

# Aggregated list of all PSP-specific libraries for convenience.
set(PSP_LIBRARIES
    stdc++
    pthread-psp
    GL
    pspaudiolib
    pspaudio
    m
    g
    c
    pspvfpu
    pspfpu
    pspgum
    pspgu
    psprtc
    pspdebug
    pspdisplay
    pspge
    pspctrl
    pspsdk
    c
    pspnet
    pspnet_inet
    pspnet_apctl
    pspnet_resolver
    psputility
    psppower
    pspuser
    pspkernel
)

SET(
    CMAKE_CXX_STANDARD_LIBRARIES
    "${CMAKE_CXX_STANDARD_LIBRARIES} \
     -lstdc++ \
    -lpthread-psp \
    -lpspaudiolib \
    -lpspaudio \
    -lm \
    -lg \
    -lc \
    -lpspvfpu \
    -lpspfpu \
    -lpspgum \
    -lpspgu \
    -lpsprtc \
    -lpspdebug \
    -lpspdisplay \
    -lpspge \
    -lpspctrl \
    -lpspsdk \
    -lc \
    -lpspnet \
    -lpspnet_inet \
    -lpspnet_apctl \
    -lpspnet_resolver \
    -lpsputility \
    -lpsppower \
    -lpspuser \
    -lpspkernel"
)

# File defining macro outputting PSP-specific EBOOT.PBP out of passed executable target:
include("${CMAKE_CURRENT_LIST_DIR}/CreatePBP.cmake")

# Helper variable for multi-platform projects to identify current platform:
set(PLATFORM_PSP TRUE BOOL)
