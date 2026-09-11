# Builds glib + gobject (and only those -- not gio/gmodule/gthread, which
# glib's own meson.build always configures but which we never ask ninja to
# actually compile) from our own vendored copy at deps/glib, instead of
# depending on whatever glib happens to be installed system-wide. This is
# needed for two reasons:
#
#  - ports/vala's generated code (see tools/cgen/cgen_lib/vapi_emitter.py)
#    is real GObject-based Vala, and the whole point of vendoring our own
#    copy is to make that buildable on platforms with no system glib at
#    all -- Dreamcast/PSP -- by cross-compiling deps/glib with the same
#    toolchain used for the rest of Simulant, rather than requiring one to
#    already exist on the target.
#  - Even on desktop, building against a *known* glib version (rather than
#    whatever a given Linux distro ships) keeps ports/vala reproducible.
#
# glib itself builds with Meson, not CMake, so this drives `meson`/`ninja`
# directly via execute_process()/add_custom_command() and exposes the
# result as ordinary IMPORTED CMake targets (`SimulantGLib::glib`,
# `SimulantGLib::gobject`) that ports/vala and samples/flappy link against
# like any other library -- see simulant_build_glib() below.
#
# Only glib+gobject are requested from ninja (not gio/gmodule/gthread,
# which glib's own meson.build unconditionally configures alongside them
# with no option to skip -- see meson.build's subdir() calls): our
# generated Vala code never uses GIO/GModule, and skipping their
# compilation entirely keeps the dependency footprint (and, for
# cross-compiles, the amount of new platform surface glib needs to work
# on) as small as possible. Meson still *evaluates* gio/gmodule's
# meson.build at configure time (their dependency() calls are consulted
# for feature auto-detection), but none of their sources are ever handed
# to the compiler.

# Captured here, at file-parse time, rather than read via
# CMAKE_CURRENT_LIST_DIR from inside simulant_build_glib() below:
# CMAKE_CURRENT_LIST_DIR inside a function() body tracks whichever file
# is *calling* the function, not the file the function was defined in --
# ports/vala/CMakeLists.txt and samples/flappy/CMakeLists.txt both
# include() this file and immediately call simulant_build_glib() from
# their own directory, so CMAKE_CURRENT_LIST_DIR read inside the function
# body would silently resolve to ports/vala/ or samples/flappy/ instead
# of cmake/ (confirmed by hitting exactly this while testing the PSP
# path standalone -- it went looking for stage_glib_for_target.py in the
# wrong directory entirely).
set(SIMULANT_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(simulant_build_glib)
  if(TARGET SimulantGLib::glib)
    return()  # already set up by an earlier call (ports/vala and samples/flappy both need it)
  endif()

  set(glib_source_dir "${CMAKE_SOURCE_DIR}/deps/glib")
  set(glib_build_dir "${CMAKE_BINARY_DIR}/deps/glib-build")

  if(NOT EXISTS "${glib_source_dir}/meson.build")
    message(FATAL_ERROR
      "deps/glib is missing or empty -- it's an in-tree vendored copy of "
      "glib, not a git submodule, so this shouldn't normally happen; check "
      "your checkout.")
  endif()

  find_program(MESON_EXECUTABLE meson REQUIRED)
  find_program(NINJA_EXECUTABLE ninja REQUIRED)

  # Options common to every platform: only glib+gobject are ever requested
  # from ninja (see module comment above), so everything here is about
  # keeping the *configure* step itself lightweight and free of optional
  # dependencies (introspection, documentation, tracing, ...) that would
  # either need tools we don't want as a build requirement or simply don't
  # exist when cross-compiling. `--force-fallback-for` pins glib's two
  # real external dependencies (PCRE2, for GRegex; libffi, for GObject
  # closure/signal marshalling) to Meson's own bundled/downloaded copies
  # instead of whatever (if anything) is installed system-wide -- the
  # same "our own copy, not the system's" reasoning as glib itself, and
  # the only way either dependency exists at all when cross-compiling.
  set(glib_setup_args
    --default-library=static
    -Dtests=false
    -Dinstalled_tests=false
    -Dnls=disabled
    -Ddocumentation=false
    -Dgtk_doc=false
    -Dman=false
    -Dman-pages=disabled
    -Ddtrace=disabled
    -Dsystemtap=disabled
    -Dsysprof=disabled
    -Dintrospection=disabled
    -Dselinux=disabled
    -Dlibmount=disabled
    -Dxattr=false
    -Dlibelf=disabled
    -Dmultiarch=false
  )

  # Assembled into a single --force-fallback-for=... below, after the
  # per-platform block has had a chance to add to it (MinGW and Android
  # both add `intl`; see each for why).
  set(glib_fallback_deps libpcre2-8 libffi)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND glib_setup_args --buildtype=debug)
  else()
    list(APPEND glib_setup_args --buildtype=release)
  endif()

  set(glib_setup_env "")
  if(PLATFORM_PSP)
    # PSP's MIPS "eabi" target can't generate position-independent code
    # at all (confirmed by cross-compiling: `cc1: error: cannot generate
    # position-independent code for '-mabi=eabi'`) -- Meson's default of
    # building static libraries as PIC (so they *could* be relinked into
    # a shared library later) needs disabling outright for this target.
    list(APPEND glib_setup_args -Db_staticpic=false)

    # Real GNOME/glib has no idea PSP exists, so this cross-compile needs
    # a small, real set of patches on top -- see stage_glib_for_target.py's
    # own docstring and cmake/glib-patches/psp/ for exactly what and why.
    # These apply to a *staged copy* of deps/glib, never the vendored
    # tree itself (see that docstring for why).
    set(glib_psp_staged_dir "${CMAKE_BINARY_DIR}/deps/glib-psp-src")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    if(NOT EXISTS "${glib_psp_staged_dir}/meson.build")
      message(STATUS "Staging a patched deps/glib for PSP (${glib_psp_staged_dir})")
      execute_process(
        COMMAND ${Python3_EXECUTABLE} ${SIMULANT_CMAKE_DIR}/stage_glib_for_target.py
                ${glib_source_dir} ${glib_psp_staged_dir} ${SIMULANT_CMAKE_DIR}/glib-patches/psp
        RESULT_VARIABLE glib_stage_result
      )
      if(NOT glib_stage_result EQUAL 0)
        message(FATAL_ERROR "Failed to stage deps/glib for PSP (exit ${glib_stage_result})")
      endif()
    endif()
    set(glib_source_dir "${glib_psp_staged_dir}")

    include(${SIMULANT_CMAKE_DIR}/GLibCrossFilePSP.cmake)
    simulant_write_glib_cross_file_psp("${CMAKE_BINARY_DIR}/deps/glib-psp-cross.ini")
    list(APPEND glib_setup_args --cross-file "${CMAKE_BINARY_DIR}/deps/glib-psp-cross.ini")
  elseif(PLATFORM_DREAMCAST)
    # KOS compiles everything -fno-PIC -fno-PIE, so the same reasoning as
    # PSP applies: Meson would otherwise build these static libraries as
    # PIC on the assumption they might be relinked into a shared library.
    list(APPEND glib_setup_args -Db_staticpic=false)

    # Same staged-copy approach as PSP -- glib upstream has never heard of
    # Dreamcast either. The patch set is smaller here (no libffi patches
    # at all; its SH backend builds as-is); see
    # cmake/glib-patches/dreamcast/ and stage_glib_for_target.py.
    set(glib_dc_staged_dir "${CMAKE_BINARY_DIR}/deps/glib-dreamcast-src")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    if(NOT EXISTS "${glib_dc_staged_dir}/meson.build")
      message(STATUS "Staging a patched deps/glib for Dreamcast (${glib_dc_staged_dir})")
      execute_process(
        COMMAND ${Python3_EXECUTABLE} ${SIMULANT_CMAKE_DIR}/stage_glib_for_target.py
                ${glib_source_dir} ${glib_dc_staged_dir} ${SIMULANT_CMAKE_DIR}/glib-patches/dreamcast
        RESULT_VARIABLE glib_stage_result
      )
      if(NOT glib_stage_result EQUAL 0)
        message(FATAL_ERROR "Failed to stage deps/glib for Dreamcast (exit ${glib_stage_result})")
      endif()
    endif()
    set(glib_source_dir "${glib_dc_staged_dir}")

    include(${SIMULANT_CMAKE_DIR}/GLibCrossFileDreamcast.cmake)
    simulant_write_glib_cross_file_dreamcast("${CMAKE_BINARY_DIR}/deps/glib-dreamcast-cross.ini")
    list(APPEND glib_setup_args --cross-file "${CMAKE_BINARY_DIR}/deps/glib-dreamcast-cross.ini")
  elseif(ANDROID)
    # One glib build per ABI; AGP gives each its own CMAKE_BINARY_DIR, so
    # glib_build_dir above is already ABI-specific. See
    # GLibCrossFileAndroid.cmake.
    include(${SIMULANT_CMAKE_DIR}/GLibCrossFileAndroid.cmake)
    simulant_write_glib_cross_file_android("${CMAKE_BINARY_DIR}/deps/glib-android-cross.ini")
    list(APPEND glib_setup_args --cross-file "${CMAKE_BINARY_DIR}/deps/glib-android-cross.ini")

    # bionic has no gettext, so Meson would fall back to proxy-libintl on
    # its own anyway; forcing it keeps that deterministic rather than
    # depending on what a given NDK sysroot happens to ship.
    list(APPEND glib_fallback_deps intl)
  elseif(MINGW)
    # Linux host, Windows target. Needs far less than PSP/Dreamcast (glib
    # supports Windows upstream) -- just a cross file, so Meson stops
    # treating this as a native build. See GLibCrossFileMinGW.cmake.
    include(${SIMULANT_CMAKE_DIR}/GLibCrossFileMinGW.cmake)
    simulant_write_glib_cross_file_mingw("${CMAKE_BINARY_DIR}/deps/glib-mingw-cross.ini")
    list(APPEND glib_setup_args --cross-file "${CMAKE_BINARY_DIR}/deps/glib-mingw-cross.ini")

    # Unlike PSP/Dreamcast (where nothing provides `intl` so Meson falls
    # back on its own), a MinGW sysroot usually *does* have one, via
    # Fedora's mingw64-gettext -- but only as a DLL import library, which
    # would make every consumer of libsimulant-vala.a need libintl-8.dll
    # at runtime. Everything else we ship for Windows is statically
    # linked, so force the same proxy-libintl subproject the consoles use:
    # it's a stub implementation, which is all that's needed here given
    # -Dnls=disabled means no translation actually happens either way.
    list(APPEND glib_fallback_deps intl)
  else()
    # Native (host) build: force meson to use the exact same compiler
    # CMake itself was configured with, rather than whatever `cc`/`c++`
    # resolve to in the ambient environment.
    set(glib_setup_env
      "CC=${CMAKE_C_COMPILER}"
      "CXX=${CMAKE_CXX_COMPILER}"
    )
  endif()

  list(JOIN glib_fallback_deps "," glib_fallback_deps_csv)
  list(APPEND glib_setup_args "--force-fallback-for=${glib_fallback_deps_csv}")

  if(NOT EXISTS "${glib_build_dir}/build.ninja")
    message(STATUS "Configuring deps/glib with Meson (${glib_build_dir})")
    # Only wrap with `cmake -E env` when there's actually an override to
    # apply (the host/native path sets CC/CXX; PSP doesn't, using a
    # --cross-file instead) -- an empty glib_setup_env still expands to
    # one stray empty-string argument (CMake's `set(var "")` makes a
    # single-empty-string list, not a zero-length one), which `cmake -E
    # env ""` seems to tolerate on its own, but there's no reason to feed
    # a subprocess chain that ends in Meson possibly shelling out to git
    # (see stage_glib_for_target.py's module docstring for the exact
    # flakiness this was found chasing) an environment invocation it
    # doesn't need at all.
    if(glib_setup_env)
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E env ${glib_setup_env}
                ${MESON_EXECUTABLE} setup ${glib_build_dir} ${glib_source_dir} ${glib_setup_args}
        RESULT_VARIABLE glib_setup_result
      )
    else()
      execute_process(
        COMMAND ${MESON_EXECUTABLE} setup ${glib_build_dir} ${glib_source_dir} ${glib_setup_args}
        RESULT_VARIABLE glib_setup_result
      )
    endif()
    if(NOT glib_setup_result EQUAL 0)
      message(FATAL_ERROR "Failed to configure deps/glib with Meson (exit ${glib_setup_result})")
    endif()
  endif()

  set(glib_lib "${glib_build_dir}/glib/libglib-2.0.a")
  set(gobject_lib "${glib_build_dir}/gobject/libgobject-2.0.a")
  set(glibconfig_h "${glib_build_dir}/glib/glibconfig.h")
  set(glib_enumtypes_h "${glib_build_dir}/gobject/glib-enumtypes.h")

  # `--force-fallback-for` (above) makes glib build PCRE2 (for GRegex) and
  # libffi (for GObject closure/signal marshalling) as Meson subprojects
  # rather than depending on a system copy of either -- but building
  # libglib-2.0.a/libgobject-2.0.a as *static* libraries doesn't require
  # resolving their symbol references (ar doesn't link), so ninja only
  # actually builds these two as a side effect of something *else*
  # depending on them; nothing does, since we deliberately don't build
  # gio/gmodule (glib's own consumers of them). They have to be requested
  # explicitly here so they exist for samples/flappy's *executable* link
  # step, which does need every symbol resolved. Paths come from each
  # subproject's own wrap-pinned version (see deps/glib/subprojects/
  # pcre2.wrap and libffi.wrap) -- update these two lines if a future
  # glib bump changes either pinned version.
  set(pcre2_lib "${glib_build_dir}/subprojects/pcre2-10.46/libpcre2-8.a")
  set(libffi_lib "${glib_build_dir}/subprojects/libffi-3.5.2/src/libffi.a")
  set(glib_ninja_targets
    glib/libglib-2.0.a gobject/libgobject-2.0.a
    subprojects/pcre2-10.46/libpcre2-8.a
    subprojects/libffi-3.5.2/src/libffi.a
  )
  set(glib_extra_libs "")

  # PSP/Dreamcast get proxy-libintl because nothing on those platforms
  # provides `intl` at all; MinGW gets it because it's forced above, to
  # avoid a libintl-8.dll runtime dependency. Either way the archive has
  # to be built and linked explicitly, same as pcre2/libffi.
  if(PLATFORM_PSP OR PLATFORM_DREAMCAST OR MINGW OR ANDROID)
    # glib/ggettext.c (compiled unconditionally, regardless of -Dnls) is a
    # thin gettext-*style* wrapper that still needs a real `intl`
    # dependency to link against even with translations disabled --
    # Meson's dependency('intl') finds one built into glibc on Linux (so
    # the host build never needs this), but neither PSPSDK's nor KOS's
    # newlib has any such thing, so it falls back to fetching Meson's own
    # `proxy-libintl` subproject wrap instead (confirmed on both: without
    # explicitly building *and linking* this too, the final executable
    # link fails on g_libintl_bindtextdomain() and friends, exactly like
    # pcre2/libffi above -- static libraries only pull in what's
    # requested).
    set(libintl_lib "${glib_build_dir}/subprojects/proxy-libintl-0.5/libintl.a")
    list(APPEND glib_ninja_targets subprojects/proxy-libintl-0.5/libintl.a)
  endif()

  # Deliberately only these ninja targets (plus whatever *they*
  # transitively depend on) -- never a bare `ninja`/`meson compile` with
  # no target list, which would also build gio/gmodule/gthread and every
  # dev tool glib defines.
  # MAKEFLAGS is cleared for the nested ninja on purpose. Under a parallel
  # `make -j`, GNU Make advertises its jobserver through MAKEFLAGS
  # (--jobserver-auth=<r>,<w>), but it only actually passes those file
  # descriptors to recipe lines it considers recursive. This one isn't, so
  # a jobserver-aware ninja (the pip-installed builds are) sees the
  # advertisement, tries to use descriptors it never inherited, and dies
  # with "Could not initialize jobserver: Invalid file descriptors" before
  # compiling anything. Hiding MAKEFLAGS makes ninja fall back to picking
  # its own job count, which is what we want here anyway.
  add_custom_command(
    OUTPUT ${glib_lib} ${gobject_lib} ${glibconfig_h} ${glib_enumtypes_h} ${pcre2_lib} ${libffi_lib} ${libintl_lib}
    COMMAND ${CMAKE_COMMAND} -E env MAKEFLAGS= ${NINJA_EXECUTABLE} -C ${glib_build_dir} ${glib_ninja_targets}
    WORKING_DIRECTORY ${glib_build_dir}
    COMMENT "Building glib + gobject (deps/glib) for ${CMAKE_SYSTEM_NAME}"
    VERBATIM
  )
  add_custom_target(SimulantGLibBuild ALL DEPENDS ${glib_lib} ${gobject_lib} ${glibconfig_h} ${glib_enumtypes_h} ${pcre2_lib} ${libffi_lib} ${libintl_lib})

  # include_directories() mirrors exactly what glib's own meson.build
  # exposes via declare_dependency() for libglib_dep/libgobject_dep (see
  # configinc/glibinc/gobjectinc in deps/glib/meson.build and
  # deps/glib/gobject/meson.build): the checkout root (for e.g.
  # `#include <gobject/gobject.h>` from within <glib/glib-object.h>) plus
  # the glib/ and gobject/ subdirectories in both source and build trees
  # (the latter for the generated glibconfig.h/glib-enumtypes.h).
  add_library(SimulantGLib::pcre2 STATIC IMPORTED GLOBAL)
  set_target_properties(SimulantGLib::pcre2 PROPERTIES IMPORTED_LOCATION "${pcre2_lib}")
  add_dependencies(SimulantGLib::pcre2 SimulantGLibBuild)

  add_library(SimulantGLib::libffi STATIC IMPORTED GLOBAL)
  set_target_properties(SimulantGLib::libffi PROPERTIES IMPORTED_LOCATION "${libffi_lib}")
  add_dependencies(SimulantGLib::libffi SimulantGLibBuild)

  set(glib_interface_link_libraries SimulantGLib::pcre2)

  if(PLATFORM_PSP OR PLATFORM_DREAMCAST OR MINGW OR ANDROID)
    add_library(SimulantGLib::libintl STATIC IMPORTED GLOBAL)
    set_target_properties(SimulantGLib::libintl PROPERTIES IMPORTED_LOCATION "${libintl_lib}")
    add_dependencies(SimulantGLib::libintl SimulantGLibBuild)
    list(APPEND glib_interface_link_libraries SimulantGLib::libintl)
  endif()

  if(PLATFORM_DREAMCAST)
    # The iconv KOS's newlib declares but doesn't implement. Already built
    # by simulant_write_glib_cross_file_dreamcast() (it has to exist before
    # Meson runs, so dependency('iconv') can find it); linked here so the
    # final executable gets the implementation too, not just glib's own
    # configure-time probes. See cmake/dreamcast-shims/dc_iconv.c.
    add_library(SimulantGLib::dciconv STATIC IMPORTED GLOBAL)
    set_target_properties(SimulantGLib::dciconv PROPERTIES
      IMPORTED_LOCATION "${SIMULANT_DC_ICONV_LIB}")
    list(APPEND glib_interface_link_libraries SimulantGLib::dciconv)

    # POSIX functions KOS doesn't implement, which only surface as
    # undefined references at the final executable link -- see
    # dreamcast-shims/dc_glib_stubs.c for which and why. Our own C file,
    # compiled by CMake with the Dreamcast toolchain rather than by Meson,
    # exactly like SimulantGLibPSPStubs below.
    add_library(SimulantGLibDreamcastStubs STATIC
      "${SIMULANT_CMAKE_DIR}/dreamcast-shims/dc_glib_stubs.c")
    list(APPEND glib_interface_link_libraries SimulantGLibDreamcastStubs)
  endif()

  if(PLATFORM_PSP)
    # PSPSDK's newlib *declares* several POSIX functions glib references
    # (process/signal/user-database calls it never actually needs to
    # succeed, just link) with no implementation at all, and doesn't
    # supply the cache-flush primitive libffi's MIPS closure trampoline
    # needs either -- see psp-shims/psp_glib_runtime_stubs.c for exactly
    # which symbols and why. This is our own C file, compiled directly by
    # CMake/the PSP toolchain file (not by Meson), unlike everything else
    # in this module.
    add_library(SimulantGLibPSPStubs STATIC "${SIMULANT_CMAKE_DIR}/psp-shims/psp_glib_runtime_stubs.c")
    list(APPEND glib_interface_link_libraries SimulantGLibPSPStubs)
  endif()

  add_library(SimulantGLib::glib STATIC IMPORTED GLOBAL)
  set_target_properties(SimulantGLib::glib PROPERTIES
    IMPORTED_LOCATION "${glib_lib}"
    INTERFACE_INCLUDE_DIRECTORIES
      "${glib_source_dir};${glib_source_dir}/glib;${glib_build_dir};${glib_build_dir}/glib"
    INTERFACE_LINK_LIBRARIES "${glib_interface_link_libraries}"
  )
  add_dependencies(SimulantGLib::glib SimulantGLibBuild)

  add_library(SimulantGLib::gobject STATIC IMPORTED GLOBAL)
  set_target_properties(SimulantGLib::gobject PROPERTIES
    IMPORTED_LOCATION "${gobject_lib}"
    INTERFACE_INCLUDE_DIRECTORIES
      "${glib_source_dir}/gobject;${glib_build_dir}/gobject"
    INTERFACE_LINK_LIBRARIES "SimulantGLib::glib;SimulantGLib::libffi"
  )
  add_dependencies(SimulantGLib::gobject SimulantGLibBuild)

  if(PLATFORM_PSP OR PLATFORM_DREAMCAST)
    # Deliberately not find_package(Threads)/Threads::Threads on either of
    # these: that module's own compile checks run through the cross
    # compiler too and hit the exact same hardcoded "-pthread" problem this
    # file already works around for glib's *own* build (see
    # cmake/GLibCrossFilePSP.cmake and GLibCrossFileDreamcast.cmake) -- one
    # more reason to keep it contained here rather than risk it leaking
    # onto simulant-vala/flappy's own link lines. Both platforms' standard
    # libraries (appended unconditionally to every target by their
    # toolchain files) already cover whatever pthread support exists:
    # KOS's pthreads live in libkallisti, which is always linked.
  elseif(MINGW)
    # The Win32 API libraries glib itself declares as its own (see
    # win32_ldflags in deps/glib/meson.build: "Win32 API libs, used only
    # by libglib and exposed in glib-2.0.pc"). Since we link glib
    # statically there's no .pc file in the picture to carry them, so
    # they have to ride on the imported target or every downstream link
    # fails on winsock/COM symbols.
    #
    # MinGW's GCC provides pthreads via winpthreads without a -pthread
    # driver flag on the link line, so no find_package(Threads) here --
    # and no libm either, which is part of the CRT on Windows.
    set_property(TARGET SimulantGLib::glib APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES ws2_32 ole32 winmm shlwapi uuid)
  else()
    find_package(Threads REQUIRED)
    set_property(TARGET SimulantGLib::glib APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES Threads::Threads)
    if(NOT WIN32)
      # glib needs libm directly (e.g. for GRegex/number formatting).
      set_property(TARGET SimulantGLib::glib APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES m)
    endif()
  endif()
endfunction()
