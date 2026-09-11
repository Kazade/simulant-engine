# Writes the Meson cross-file used to build deps/glib for Dreamcast (KOS),
# and builds the small support library that cross-file needs.
#
# Counterpart to GLibCrossFilePSP.cmake. KOS turns out to be a much better
# host for glib than PSPSDK: it has real pthreads, a real poll(), a real
# <sys/utsname.h>, and libffi's SH backend builds completely unmodified (no
# equivalent of the two hand-written MIPS assembly patches PSP needs). What
# it does need:
#
#  - -Db_staticpic=false, set by the caller. KOS builds everything
#    -fno-PIC -fno-PIE, and Meson otherwise defaults static libraries to
#    PIC so they could be relinked into a shared library later. Two of
#    glib's internal static_library() calls hardcode `pic : true` and
#    override even that, which is why the patch in
#    cmake/glib-patches/dreamcast/ has to touch them directly.
#
#  - Header shims, from cmake/dreamcast-shims (put first on the include
#    path so they can interpose). Each one is commented in place with the
#    exact reason it exists; briefly: <sys/poll.h> forwards to KOS's
#    <poll.h>, <sched.h> and <machine/time.h> re-declare functions KOS
#    implements but whose declarations sit behind C-standard/feature-macro
#    conditions that -std=gnu99 doesn't satisfy, and <sys/resource.h> adds
#    the setrlimit()/RLIMIT_* that KOS genuinely lacks.
#
#  - An iconv implementation. KOS's newlib declares iconv but is built
#    without it, and glib has no option to go without (outside Windows it
#    does a bare dependency('iconv') and stops the configure if it fails).
#    cmake/dreamcast-shims/dc_iconv.c provides a small real one; it's
#    compiled here, ahead of Meson, so that dependency() can find it.
#
#  - Compiler wrappers, to drop Meson's hardcoded `-pthread`, which this
#    toolchain rejects outright. See cmake/dreamcast-shims/bin/.
#
#  - -D_POSIX_HOST_NAME_MAX / -DSSIZE_MAX, neither of which KOS defines but
#    both of which glib uses once it sees the surrounding feature macros.
#
#  - -Wno-error=incompatible-pointer-types. GCC 14 made this an error by
#    default, and this newlib types int32_t as `long` where pcre2 uses
#    `int`. Both are 32-bit here with identical representation, so it's a
#    spelling difference rather than a real mismatch -- the same
#    ecosystem-wide GCC 14 breakage worked around on the PSP side.
#    Deliberately *not* accompanied by -Wno-error=implicit-function-
#    declaration: the missing declarations found here were all fixed
#    properly with shims, and that diagnostic should stay fatal so any
#    future one is caught rather than silently linking.

set(SIMULANT_DC_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(simulant_write_glib_cross_file_dreamcast out_path)
  if(NOT DEFINED ENV{KOS_BASE})
    message(FATAL_ERROR
      "KOS_BASE is not set -- source KallistiOS's environ.sh before "
      "configuring for Dreamcast.")
  endif()

  set(kos_base "$ENV{KOS_BASE}")
  set(kos_cc_base "$ENV{KOS_CC_BASE}")
  if(NOT kos_cc_base)
    set(kos_cc_base "${kos_base}/../sh-elf")
  endif()

  set(shims_dir "${SIMULANT_DC_CMAKE_DIR}/dreamcast-shims")
  set(work_dir "${CMAKE_BINARY_DIR}/deps/glib-dc")
  file(MAKE_DIRECTORY "${work_dir}")

  # --- compiler wrappers (strip Meson's -pthread) ------------------------
  set(SIMULANT_DC_REAL_GCC "${kos_cc_base}/bin/sh-elf-gcc")
  set(SIMULANT_DC_REAL_GXX "${kos_cc_base}/bin/sh-elf-g++")
  foreach(tool sh-elf-gcc sh-elf-g++)
    configure_file("${shims_dir}/bin/${tool}" "${work_dir}/${tool}" @ONLY)
    file(CHMOD "${work_dir}/${tool}"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endforeach()

  # --- iconv support library --------------------------------------------
  # Built before Meson runs so that dependency('iconv') can link against
  # it; the same archive is linked into the final binary via
  # SimulantGLib::dciconv (see BuildGLib.cmake).
  set(iconv_obj "${work_dir}/dc_iconv.o")
  set(iconv_lib "${work_dir}/libdciconv.a")
  execute_process(
    COMMAND ${SIMULANT_DC_REAL_GCC} -O2 -fno-PIC -fno-PIE -m4-single -ml
            -D__DREAMCAST__ -D_arch_dreamcast
            -I${kos_base}/include
            -I${kos_base}/kernel/arch/dreamcast/include
            -c ${shims_dir}/dc_iconv.c -o ${iconv_obj}
    RESULT_VARIABLE iconv_cc_result
    ERROR_VARIABLE iconv_cc_error
  )
  if(NOT iconv_cc_result EQUAL 0)
    message(FATAL_ERROR "Failed to compile the Dreamcast iconv shim:\n${iconv_cc_error}")
  endif()
  execute_process(
    COMMAND ${kos_cc_base}/bin/sh-elf-ar rcs ${iconv_lib} ${iconv_obj}
    RESULT_VARIABLE iconv_ar_result
    ERROR_VARIABLE iconv_ar_error
  )
  if(NOT iconv_ar_result EQUAL 0)
    message(FATAL_ERROR "Failed to archive the Dreamcast iconv shim:\n${iconv_ar_error}")
  endif()
  set(SIMULANT_DC_ICONV_LIB "${iconv_lib}" PARENT_SCOPE)

  # --- the cross-file itself --------------------------------------------
  # [constants] keeps the compile and link flags in one place; note both
  # c_* and cpp_* are set from them. Meson probes some dependencies
  # (iconv among them) with the C++ compiler, and with cpp_link_args unset
  # those probes link without KOS's libraries or linker script at all --
  # which is why dependency('iconv') kept failing until cpp_link_args was
  # filled in too, long after the library itself was correct.
  file(WRITE "${out_path}" "\
[binaries]
c = '${work_dir}/sh-elf-gcc'
cpp = '${work_dir}/sh-elf-g++'
ar = '${kos_cc_base}/bin/sh-elf-ar'
strip = '${kos_cc_base}/bin/sh-elf-strip'
pkg-config = 'false'

[constants]
kos_common = ['-O2', '-fno-PIC', '-fno-PIE', '-fomit-frame-pointer', '-m4-single', '-ml', '-mfsrra', '-mfsca', '-ffunction-sections', '-fdata-sections', '-matomic-model=soft-gusa', '-ftls-model=local-exec', '-D__DREAMCAST__', '-D_arch_dreamcast', '-D_arch_sub_pristine', '-D_POSIX_HOST_NAME_MAX=255', '-DSSIZE_MAX=2147483647', '-Wno-error=incompatible-pointer-types', '-I${shims_dir}', '-I${kos_base}/include', '-I${kos_base}/kernel/arch/dreamcast/include', '-I${kos_base}/addons/include/', '-I${kos_base}/../kos-ports/include']
kos_link = ['-m4-single', '-ml', '-Wl,--gc-sections', '-T${kos_base}/utils/ldscripts/shlelf.xc', '-nostdlib', '-L${work_dir}', '-L${kos_base}/lib/dreamcast', '-L${kos_base}/addons/lib/dreamcast', '-L${kos_base}/../kos-ports/lib', '-Wl,--start-group', '-ldciconv', '-lkallisti', '-lm', '-lc', '-lgcc', '-Wl,--end-group']

[built-in options]
c_args = kos_common
cpp_args = kos_common
c_link_args = kos_link
cpp_link_args = kos_link

[host_machine]
system = 'dreamcast'
cpu_family = 'sh4'
cpu = 'sh4'
endian = 'little'

[properties]
needs_exe_wrapper = true
")
endfunction()
