# Generates a Meson "cross file" for cross-compiling deps/glib with the
# MinGW-w64 toolchain (Linux host, Windows target -- see
# platforms/windows/Dockerfile and the build:windows-x64-mingw:* CI jobs,
# which drive CMake through Fedora's `mingw64-cmake` wrapper).
#
# Unlike PSP and Dreamcast, Windows is a first-class glib target upstream,
# so this needs none of the supporting machinery those two do: no header
# shims, no patches to glib itself, no compiler wrapper scripts (MinGW's
# GCC accepts Meson's hardcoded `-pthread` happily, via winpthreads), and
# no iconv implementation (glib uses its own win_iconv on Windows).
#
# What it *does* need is simply to be told it's cross-compiling at all.
# Without a cross file, Meson assumes a native build and leaves
# host_system as 'linux' while the compiler happily reports winsock2.h as
# present -- glib then takes a half-Windows/half-POSIX path through its
# own meson.build and dies with:
#
#     meson.build:2006:48: ERROR: Entry _WIN32_WINNT not in configuration data.
#
# because the block that populates _WIN32_WINNT is guarded on
# host_system == 'windows'. Everything below exists to make that check
# (and the many others like it) answer correctly.
#
# All the toolchain paths come from the variables CMake has already
# resolved for this toolchain rather than being reconstructed here, so
# this automatically follows whatever mingw64-cmake (or an explicit
# -DCMAKE_TOOLCHAIN_FILE) configured.
function(simulant_write_glib_cross_file_mingw out_path)
  if(NOT MINGW)
    message(FATAL_ERROR "simulant_write_glib_cross_file_mingw() called for a non-MinGW toolchain")
  endif()

  # Meson wants the pkg-config that reports the *cross* sysroot's
  # libraries. Fedora's mingw packaging ships one next to the compiler
  # (x86_64-w64-mingw32-pkg-config); if it somehow isn't there, disable
  # pkg-config entirely rather than let Meson fall back to the host's and
  # start finding host-architecture libraries -- the same reasoning as the
  # PSP cross file's `pkg-config = 'false'`.
  string(REGEX REPLACE "gcc(-[0-9.]+)?$" "" mingw_tool_prefix "${CMAKE_C_COMPILER}")
  set(mingw_pkg_config "${mingw_tool_prefix}pkg-config")
  if(NOT EXISTS "${mingw_pkg_config}")
    set(mingw_pkg_config "false")
  endif()

  # Meson spells these 'x86_64' and 'x86'; taken from the pointer width
  # CMake probed rather than the triple, so a 32-bit mingw32 build works
  # without special-casing the prefix string.
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(mingw_cpu_family "x86_64")
    set(mingw_cpu "x86_64")
  else()
    set(mingw_cpu_family "x86")
    set(mingw_cpu "i686")
  endif()

  file(WRITE "${out_path}" "\
[binaries]
c = '${CMAKE_C_COMPILER}'
cpp = '${CMAKE_CXX_COMPILER}'
ar = '${CMAKE_AR}'
strip = '${CMAKE_STRIP}'
windres = '${CMAKE_RC_COMPILER}'
pkg-config = '${mingw_pkg_config}'

[host_machine]
system = 'windows'
cpu_family = '${mingw_cpu_family}'
cpu = '${mingw_cpu}'
endian = 'little'

[properties]
# Windows executables can't be run on the Linux host doing the
# cross-compile, so Meson can't execute its little probe programs and
# falls back to the conservative answer for every run-time check -- the
# same constraint the PSP and Dreamcast cross files already live with.
# (The CI image does have Wine installed, for running the *test suite*
# later, but wiring it in as an exe_wrapper here would make glib's
# configure results depend on Wine's fidelity, which isn't a trade worth
# making for a dependency we only ever link statically.)
needs_exe_wrapper = true
")
endfunction()
