# C-linkage bindings for Simulant (libsimulant-c): a plain-C API wrapping
# the public C++ classes, so Simulant can be used from any language with a
# C FFI. The .h/.cpp files under generated/ are produced by
# tools/cgen/cgen.py (see tools/cgen/README.md) and are committed to the
# repo, so building simulant-c does *not* require Python or libclang --
# only re-generating them does.
#
# include/ and src/ hold hand-written additions cgen.py can't produce
# mechanically -- e.g. stage_node_ext.h/.cpp, which lets C code define its
# own StageNode "subclass" via a vtable of callbacks (see that header for
# usage). runtime/ holds internal-only helpers used by generated code
# (string ownership, ...) that aren't part of the public API surface.

file(GLOB SIMULANT_C_GENERATED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/generated/src/*.cpp)

# sdl2_window.cpp is mechanically generated because cgen.py scans every
# header reachable from simulant.h on the *host* running it (normally
# Linux), with no notion of which platform the result will actually be
# compiled for -- smlt::SDL2Window is real and wrapped correctly, but
# only actually part of the engine's own build on desktop-ish platforms
# (see the SDL2Window/sdl2_window.cpp entry in simulant/CMakeLists.txt's
# own platform if/elseif chain: Dreamcast/PSP/Android all use their own
# native window class instead). Compiling the generated wrapper for one
# of those platforms fails immediately on `#include <SDL.h>`, which
# doesn't exist there -- same reasoning, same fix as that file.
if(PLATFORM_DREAMCAST OR PLATFORM_PSP OR ANDROID)
  list(FILTER SIMULANT_C_GENERATED_SOURCES EXCLUDE REGEX "/sdl2_window\\.cpp$")
endif()

file(GLOB SIMULANT_C_HANDWRITTEN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
set(SIMULANT_C_RUNTIME_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/runtime/smlt_c_util.cpp)

if(NOT SIMULANT_C_GENERATED_SOURCES)
  message(WARNING "ports/c/generated/src is empty -- run 'python3 tools/cgen/cgen.py' "
                   "to generate the C bindings sources before building simulant-c")
endif()

add_library(simulant-c STATIC
  ${SIMULANT_C_GENERATED_SOURCES}
  ${SIMULANT_C_HANDWRITTEN_SOURCES}
  ${SIMULANT_C_RUNTIME_SOURCES}
)

target_include_directories(simulant-c
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(simulant-c PRIVATE simulant)

set_target_properties(simulant-c PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  OUTPUT_NAME "simulant-c"
)

# Optional developer target: re-scans the Simulant C++ headers with
# tools/cgen/cgen.py and refreshes ports/c/generated/ *and*
# ports/vala/simulant-c.vapi (cgen.py writes both by default -- see
# ports/vala/README.md). Requires Python 3 with the `libclang` package
# installed (pip install libclang). This is never required for the
# default build -- only run it after changing the public API and re-run
# CMake afterwards so any new/removed generated files are picked up.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_custom_target(simulant-c-regenerate
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/cgen/cgen.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Regenerating Simulant C bindings with tools/cgen/cgen.py"
    VERBATIM
  )
endif()
