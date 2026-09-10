# Vala bindings for Simulant: simulant-c.vapi describes libsimulant-c (see
# ../c/) to the Vala compiler. It's produced by tools/cgen/cgen.py (the
# same run that generates ../c/generated/, via its --vapi flag, on by
# default) and committed to the repo like the C sources are, so using it
# from a Vala project needs nothing beyond `valac --vapidir=ports/vala
# --pkg simulant-c` plus linking against simulant-c/simulant -- no Python,
# libclang, or Vala compiler required just to consume it.
#
# See README.md for the naming/ownership conventions this maps C onto.

find_program(VALAC_EXECUTABLE valac)
if(VALAC_EXECUTABLE)
  # simulant-vala: a small library, written in real Vala (not just a
  # .vapi), giving Application/Scene/SceneManager/StageNode (and its
  # whole built-in node hierarchy: Sprite, Camera2d, ...) real,
  # subclassable Vala classes -- see the module docstring in
  # tools/cgen/cgen_lib/vapi_emitter.py for why [Compact] (everything else
  # in simulant-c.vapi) can't support that. Every one of these classes is
  # entirely mechanically generated into generated/ (committed to the
  # repo like ../c/generated/ and simulant-c.vapi), with any genuinely
  # hand-written logic (virtual hooks, non-trivial constructors, ...)
  # spliced in from runtime/custom/<name>.vala at generation time -- see
  # HAND_WRITTEN_VALA_CLASSES in vapi_emitter.py to add a new one.
  #
  # valac's own `--library` mode transpiles the sources below to C *and*
  # writes a matching simulant-vala.vapi/.h describing the result, the
  # same way any other Vala library is consumed -- so using this from
  # another Vala project needs `--vapidir <this build dir>
  # --pkg simulant-vala` alongside `--pkg simulant-c`, plus linking
  # libsimulant-vala.a in with libsimulant-c.a/libsimulant.

  file(GLOB SIMULANT_VALA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.vala)

  set(SIMULANT_VALA_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated-c)
  set(SIMULANT_VALA_VAPI ${CMAKE_CURRENT_BINARY_DIR}/simulant-vala.vapi)
  set(SIMULANT_VALA_HEADER ${CMAKE_CURRENT_BINARY_DIR}/simulant-vala.h)

  # One .c per .vala input (valac's usual naming), mirroring the pattern
  # samples/flappy/CMakeLists.txt already uses for its own transpile step.
  set(SIMULANT_VALA_GENERATED_SOURCES "")
  foreach(vala_src ${SIMULANT_VALA_SOURCES})
    get_filename_component(vala_name ${vala_src} NAME_WE)
    list(APPEND SIMULANT_VALA_GENERATED_SOURCES ${SIMULANT_VALA_GENERATED_DIR}/${vala_name}.c)
  endforeach()

  add_custom_command(
    OUTPUT ${SIMULANT_VALA_GENERATED_SOURCES} ${SIMULANT_VALA_VAPI} ${SIMULANT_VALA_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMULANT_VALA_GENERATED_DIR}
    COMMAND ${VALAC_EXECUTABLE}
            --vapidir=${CMAKE_CURRENT_SOURCE_DIR}
            --pkg simulant-c --pkg simulant-c-ext
            --library=simulant-vala
            -H ${SIMULANT_VALA_HEADER}
            -C --directory=${SIMULANT_VALA_GENERATED_DIR}
            --basedir=${CMAKE_CURRENT_SOURCE_DIR}/generated
            ${SIMULANT_VALA_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/generated-c/simulant-vala.vapi ${SIMULANT_VALA_VAPI}
    DEPENDS ${SIMULANT_VALA_SOURCES}
            ${CMAKE_CURRENT_SOURCE_DIR}/simulant-c.vapi
            ${CMAKE_CURRENT_SOURCE_DIR}/simulant-c-ext.vapi
    COMMENT "Transpiling ports/vala/generated to C (simulant-vala)"
    VERBATIM
  )

  add_library(simulant-vala STATIC ${SIMULANT_VALA_GENERATED_SOURCES})
  target_include_directories(simulant-vala PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(simulant-vala PUBLIC simulant-c simulant)

  # valac deliberately emits its vtable dispatch wrappers with a void*
  # first parameter and assigns them straight to our typed function
  # pointer fields, e.g.
  #
  #     void __smlt_scene_dispatch_on_load_...(void* _self_, void* user_data);
  #     vtable.on_load = __smlt_scene_dispatch_on_load_...;   /* wants smlt_scene_t* */
  #
  # relying on those having identical representation, which they do. GCC
  # 14 made -Wincompatible-pointer-types an error by default, so whether
  # this builds comes down to the individual compiler's default rather
  # than anything about the code: it's a warning on the GCC 16 host here
  # and a hard error on the sh-elf GCC 15.2 used for Dreamcast.
  #
  # Demoted back to a warning rather than silenced, so a *genuine* pointer
  # mismatch in this generated code would still be visible. The real fix
  # is for cgen.py to emit casts in the vtable assignments; until then this
  # keeps the generated sources building the same way everywhere.
  if(NOT CMAKE_C_COMPILER_ID MATCHES "MSVC")
    target_compile_options(simulant-vala PRIVATE -Wno-error=incompatible-pointer-types)
  endif()

  # Our own vendored glib (deps/glib), built for whatever platform Simulant
  # itself is being built for -- see cmake/BuildGLib.cmake for why this
  # exists instead of a plain pkg_check_modules(glib-2.0 gobject-2.0)
  # against the system's glib (short version: there usually isn't one, on
  # Dreamcast/PSP).
  include(${CMAKE_SOURCE_DIR}/cmake/BuildGLib.cmake)
  simulant_build_glib()
  target_link_libraries(simulant-vala PUBLIC SimulantGLib::glib SimulantGLib::gobject)
  # Regression check, not a build dependency of anything: transpiles
  # tests/smoke_test.vala against the checked-in .vapi to catch it drifting
  # out of sync with ports/c/generated/ (e.g. after a manual edit, or a
  # cgen.py change that wasn't followed by a regenerate). Only checks that
  # valac accepts the Vala -> C translation; it doesn't compile/link/run
  # the result, so it doesn't need the C headers' full include path.
  add_custom_target(simulant-vala-check
    COMMAND ${VALAC_EXECUTABLE} --vapidir=${CMAKE_CURRENT_SOURCE_DIR}
            --pkg simulant-c -C
            --directory=${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_test.vala
    COMMENT "Checking ports/vala/simulant-c.vapi with valac"
    VERBATIM
  )
else()
  message(STATUS "valac not found -- skipping simulant-vala-check target (optional)")
endif()
