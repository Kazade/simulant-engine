# SH4 kernel generator

Generates `simulant/renderers/pvr/pvr_lighting_sh4.s`, the PVR renderer's
per-vertex kernels: pass 1 (clip-space transform), lighting (geometry,
directional and point lights, combine) and pack (perspective divide and
packing into PVR vertices).

Each kernel is a software-pipelined loop. Several vertices are in flight at
once, so the FPU's long latencies (FTRV, FSRRA) are filled with other
vertices' work. The schedules come from a constraint solver, not by hand.

## Files

| file | purpose |
|---|---|
| `lightgen.py` | The kernels: per-vertex op lists, register sets, and the surrounding asm. Prints the `.s` file. |
| `modsched.py` | Modulo scheduler (OR-Tools CP-SAT) and loop code generation (prologue, kernel, epilogue). |
| `sh4timing.py` | SH7091 timing model, from the measured tables at <https://ornio.nilware.io/sh4-sim/timings/>. Also simulates a generated loop: `python tools/sh4_asm/sh4timing.py <file.s>`. |
| `search_ii.py` | Finds the smallest II (cycles per vertex) each kernel can be scheduled at. |
| `*_ii<II>.json` | Cached schedules. The solver isn't deterministic, so these keep the output stable. |
| `kbench/` | Hardware microbenchmark: cycles per vertex for each kernel, on a Dreamcast. |

## Regenerating

    python tools/sh4_asm/lightgen.py > simulant/renderers/pvr/pvr_lighting_sh4.s

This uses the cached schedules and needs nothing beyond Python 3.

After changing a kernel's op list or II, delete its cached schedule. The
next run re-solves it, which needs OR-Tools (`pip install ortools`). To find
the smallest feasible II after a change:

    python tools/sh4_asm/search_ii.py [kernel ...]

Then set the kernel's `*_II` constant in `lightgen.py` to the value found.

## Checking

- `sh4timing.py` predicts each generated loop's cycles per pass. It should
  equal the scheduled length (II × register sets) plus 2 cycles for the
  loop branch.
- `kbench/run.sh [asm file]` measures each kernel on hardware (via
  `dc-tool-ser` and the `kazade/dreamcast-sdk` docker image). It reports
  steady-state cycles per vertex, with call and pipeline-fill costs
  cancelled out. It also checks the point-light kernel's weights against
  the C++ it replaced (`kbench/pointc.cpp`), and times both.
- `kbench`'s numbers shift by a few cycles when its own code or data moves
  (cache placement). Compare kernels within one build of it, not across
  edits to `kbench.c`.
