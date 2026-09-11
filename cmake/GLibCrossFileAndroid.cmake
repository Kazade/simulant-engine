# Generates a Meson "cross file" for cross-compiling deps/glib against the
# Android NDK, once per ABI.
#
# Android is a supported glib target upstream (its meson.build has real
# host_system == 'android' branches), so like MinGW -- and unlike PSP and
# Dreamcast -- this needs no header shims, no patches to glib, and no
# compiler wrapper scripts. It just has to tell Meson which ABI it's
# building for.
#
# The one structural difference from every other platform here: Gradle
# builds several ABIs from one invocation (see ABI_FILTERS in
# gradle/simulant/build.gradle and the build:android-all-clang:* CI jobs,
# which pass x86_64;armeabi-v7a;arm64-v8a), running them in parallel.
# That's safe because AGP gives each ABI its own CMAKE_BINARY_DIR, so each
# gets its own glib build tree -- and where they *do* overlap, extracting
# Meson's wrap subprojects into the shared deps/glib/subprojects/, Meson
# serialises them itself through subprojects/.wraplock (verified by
# running three concurrent `meson setup`s against one source tree).
#
# Rather than reconstruct the compiler path, this uses the NDK's
# triple-prefixed clang wrappers (e.g. aarch64-linux-android28-clang).
# Those bake in both --target and the API level, which is what Meson wants;
# CMAKE_C_COMPILER can't be used directly the way the MinGW cross file
# uses it, because the NDK toolchain file points that at a bare `clang`
# and supplies the target separately through flags -- handing that to
# Meson unadorned would silently produce host binaries.
function(simulant_write_glib_cross_file_android out_path)
  if(NOT ANDROID_ABI)
    message(FATAL_ERROR "ANDROID_ABI is not set -- this should only be called when ANDROID is TRUE")
  endif()

  # ANDROID_TOOLCHAIN_ROOT is set by the NDK's own android.toolchain.cmake
  # (see toolchains/android.toolchain.cmake, which AGP uses a copy of);
  # reconstructed from the NDK path if some future NDK stops exporting it.
  set(ndk_toolchain_root "${ANDROID_TOOLCHAIN_ROOT}")
  if(NOT ndk_toolchain_root)
    if(CMAKE_ANDROID_NDK)
      set(ndk_root "${CMAKE_ANDROID_NDK}")
    else()
      set(ndk_root "${ANDROID_NDK}")
    endif()
    set(ndk_toolchain_root "${ndk_root}/toolchains/llvm/prebuilt/linux-x86_64")
  endif()

  # ABI -> (clang wrapper triple, Meson cpu_family, Meson cpu). The triple
  # is the NDK's own naming for the wrapper scripts in bin/, which is why
  # armeabi-v7a is "armv7a-linux-androideabi" rather than the
  # "arm-linux-androideabi" used for the old GCC-era toolchain names.
  if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(ndk_triple "aarch64-linux-android")
    set(meson_cpu_family "aarch64")
    set(meson_cpu "aarch64")
  elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(ndk_triple "armv7a-linux-androideabi")
    set(meson_cpu_family "arm")
    set(meson_cpu "armv7a")
  elseif(ANDROID_ABI STREQUAL "x86_64")
    set(ndk_triple "x86_64-linux-android")
    set(meson_cpu_family "x86_64")
    set(meson_cpu "x86_64")
  elseif(ANDROID_ABI STREQUAL "x86")
    set(ndk_triple "i686-linux-android")
    set(meson_cpu_family "x86")
    set(meson_cpu "i686")
  else()
    message(FATAL_ERROR "Unsupported ANDROID_ABI for the glib cross build: ${ANDROID_ABI}")
  endif()

  # The API level, as normalised by the NDK toolchain file (which accounts
  # for ABIs whose minimum is higher than the requested minSdkVersion).
  #
  # Deliberately *not* CMAKE_SYSTEM_VERSION: with NDK r27 that reads back
  # as plain "1", which silently yields a clang wrapper name like
  # aarch64-linux-android1-clang that doesn't exist. ANDROID_PLATFORM_LEVEL
  # and ANDROID_NATIVE_API_LEVEL both carry the real value (28 for this
  # project's minSdkVersion); ANDROID_PLATFORM is the "android-28" spelling.
  if(ANDROID_PLATFORM_LEVEL)
    set(ndk_api "${ANDROID_PLATFORM_LEVEL}")
  elseif(ANDROID_NATIVE_API_LEVEL)
    set(ndk_api "${ANDROID_NATIVE_API_LEVEL}")
  elseif(ANDROID_PLATFORM MATCHES "android-([0-9]+)")
    set(ndk_api "${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR
      "Could not determine the Android API level for the glib cross build "
      "(ANDROID_PLATFORM_LEVEL/ANDROID_NATIVE_API_LEVEL/ANDROID_PLATFORM are "
      "all unset or unrecognised)")
  endif()

  set(ndk_bin "${ndk_toolchain_root}/bin")

  # The triple-and-API clang wrapper names are an NDK implementation detail,
  # so check rather than assume -- getting this wrong otherwise surfaces as
  # a confusing Meson failure much later in the build.
  if(NOT EXISTS "${ndk_bin}/${ndk_triple}${ndk_api}-clang")
    message(FATAL_ERROR
      "No NDK clang wrapper at ${ndk_bin}/${ndk_triple}${ndk_api}-clang -- "
      "the ABI/API mapping in this file needs updating for this NDK.")
  endif()

  file(WRITE "${out_path}" "\
[binaries]
c = '${ndk_bin}/${ndk_triple}${ndk_api}-clang'
cpp = '${ndk_bin}/${ndk_triple}${ndk_api}-clang++'
ar = '${ndk_bin}/llvm-ar'
strip = '${ndk_bin}/llvm-strip'
# No pkg-config inside an NDK sysroot; disabled outright so Meson can't
# fall back to the host's and start finding host-architecture libraries,
# the same guard the PSP cross file uses.
pkg-config = 'false'

[host_machine]
system = 'android'
cpu_family = '${meson_cpu_family}'
cpu = '${meson_cpu}'
endian = 'little'

[properties]
# Android binaries can't run on the Linux host doing the cross-compile, so
# Meson falls back to the conservative answer for every run-time check --
# the same constraint the other cross-compiled targets here live with.
needs_exe_wrapper = true
")
endfunction()
