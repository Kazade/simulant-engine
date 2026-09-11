# Generates a Meson "cross file" for cross-compiling deps/glib against the
# PSPSDK toolchain (see toolchains/PSP.cmake, which sets PLATFORM_PSP and
# the PSPBIN/PSPDEV/PSPSDK variables this reads).
#
# Verified against a real PSPSDK install (the kazade/psp-sdk Docker image)
# -- glib+gobject (plus the bundled PCRE2/libffi it needs, see
# simulant_build_glib() in BuildGLib.cmake) cross-compile and link cleanly
# for real MIPS/PSP with the settings below, in combination with:
#  - cmake/glib-patches/psp/glib-psp-support.patch (applied to a staged
#    copy of deps/glib, never the submodule itself -- see
#    cmake/stage_glib_for_target.py, run from simulant_build_glib() in
#    BuildGLib.cmake)
#  - cmake/glib-patches/psp/libffi/*.patch (applied via libffi.wrap's own
#    diff_files, the same way deps/glib already patches its bundled pcre2)
#  - cmake/psp-shims/ (a handful of missing/incomplete PSPSDK headers --
#    sys/poll.h, sys/resource.h, sys/utsname.h, sgidefs.h -- each with its
#    own comment explaining exactly what's missing and why the fix used
#    is safe; see that directory)
#  - cmake/psp-shims/bin/psp-gcc + psp-g++, thin wrapper scripts that
#    rewrite Meson's hardcoded `-pthread` flag (see thread_flags() in
#    mesonbuild/compilers/mixins/clike.py -- not overridable from a
#    cross-file or project meson.build) into the `-fpthread` spelling
#    PSPSDK's GCC actually accepts
#
# libffi's caveat is the one piece of this that's confirmed to *compile*
# against real PSPSDK but is NOT confirmed correct at *runtime*: PSP's
# Allegrex CPU has no real double-precision FPU hardware, so libffi's O32
# MIPS trampoline (the closest match to PSP's "eabi" calling convention)
# needed its l.d/s.d instructions manually expanded to single-precision
# lwc1/swc1 pairs just to assemble at all (see cmake/glib-patches/psp/
# libffi/fix-allegrex-fpu-ops.patch). This is harmless today because
# Simulant's generated Vala code never creates a GObject GClosure/connects
# a signal via the generic marshaller -- the only things in glib/gobject
# that actually invoke a libffi trampoline -- but if that ever changes,
# treat any closure taking or returning a float/double as unverified
# until checked on real hardware.
function(simulant_write_glib_cross_file_psp out_path)
  if(NOT DEFINED PSPBIN)
    message(FATAL_ERROR "PSPBIN is not set -- this should only be called when PLATFORM_PSP is TRUE")
  endif()

  # SIMULANT_CMAKE_DIR (not CMAKE_CURRENT_LIST_DIR, which inside a
  # function body tracks the *caller* -- see BuildGLib.cmake's own
  # comment on this) is set by BuildGLib.cmake before it includes this
  # file, to cmake/'s own directory.
  set(shims_dir "${SIMULANT_CMAKE_DIR}/psp-shims")
  set(wrapper_dir "${CMAKE_BINARY_DIR}/deps/glib-psp-wrappers")
  file(MAKE_DIRECTORY "${wrapper_dir}")

  # configure_file()'s @ONLY substitutes these two into the wrapper
  # scripts below (see cmake/psp-shims/bin/psp-gcc/psp-g++).
  set(SIMULANT_PSP_REAL_GCC "${PSPBIN}/psp-gcc")
  set(SIMULANT_PSP_REAL_GXX "${PSPBIN}/psp-g++")
  configure_file("${shims_dir}/bin/psp-gcc" "${wrapper_dir}/psp-gcc" @ONLY)
  configure_file("${shims_dir}/bin/psp-g++" "${wrapper_dir}/psp-g++" @ONLY)
  file(CHMOD "${wrapper_dir}/psp-gcc" "${wrapper_dir}/psp-g++"
       PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  file(WRITE "${out_path}" "\
[binaries]
c = '${wrapper_dir}/psp-gcc'
cpp = '${wrapper_dir}/psp-g++'
ar = '${PSPBIN}/psp-ar'
strip = '${PSPBIN}/psp-strip'
# No pkg-config on a PSPSDK sysroot; disabling it outright avoids Meson
# falling back to the *host*'s pkg-config and finding host-architecture
# libraries by mistake.
pkg-config = 'false'

[built-in options]
# -I here for cmake/psp-shims/ (see the module comment above). The rest
# are all specific, real gaps found by actually cross-compiling against
# PSPSDK -- see each shim header's own comment for why it's safe:
#  - incompatible-pointer-types/int-conversion: newlib's int32_t is
#    `long`, not `int` -- same width, different type, which GCC 14+
#    treats as an error by default (pcre2_compile.c hits this).
#  - implicit-function-declaration: demoted the same way, belt-and-braces.
#  - _POSIX_HOST_NAME_MAX: PSPSDK defines _SC_HOST_NAME_MAX (routing
#    glib/ghostutils.c into the branch that wants this) but not the
#    fallback constant itself.
#  - SSIZE_MAX: not in PSPSDK's <limits.h> at all; this is literally
#    INT32_MAX, correct for any 32-bit ssize_t.
#  - __PSP__: matches the macro toolchains/PSP.cmake's add_definitions()
#    already uses everywhere else in this project; needed here too since
#    Meson's cross-compile doesn't see CMake's own add_definitions()
#    (see glib-psp-support.patch's glib-unix.c hunk).
c_args = ['-I${shims_dir}', '-Wno-error=implicit-function-declaration', '-Wno-error=incompatible-pointer-types', '-Wno-error=int-conversion', '-D_POSIX_HOST_NAME_MAX=255', '-DSSIZE_MAX=2147483647', '-D__PSP__']
cpp_args = ['-I${shims_dir}', '-D__PSP__']

[host_machine]
system = 'psp'
cpu_family = 'mips'
cpu = 'allegrex'
endian = 'little'

[properties]
# PSP executables can't run on the host performing the cross-compile, so
# Meson can't execute little test programs to probe runtime behaviour --
# every such check falls back to the conservative/POSIX-default answer
# instead. This is the same constraint every other cross-compiled
# dependency in this project's toolchains already lives with.
needs_exe_wrapper = true
")
endfunction()
