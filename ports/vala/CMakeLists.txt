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
