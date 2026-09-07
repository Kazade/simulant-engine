# cgen: C bindings generator

`cgen.py` scans Simulant's C++ headers with libclang and generates a
plain-C wrapper API (`libsimulant-c`) so the engine can be used from any
language with a C FFI (Lua, Rust, C#, Python's `ctypes`, ...).

Generated output goes to `ports/c/generated/` (committed to the repo) and
is compiled into `libsimulant-c.a` by `ports/c/CMakeLists.txt`, which is
pulled into the main build when `-DSIMULANT_BUILD_C_BINDINGS=ON` (the
default on desktop platforms; off for Dreamcast/PSP/Xbox/Android, same as
`SIMULANT_BUILD_TOOLS`).

Regenerating is a manual, opt-in developer step -- ordinary builds never
need Python or libclang, only the checked-in generated sources.

By default `cgen.py` *also* writes a Vala `.vapi` binding for the
generated C API straight from the same IR (`--vapi`, on unless
`--no-vapi` is passed) -- see `ports/vala/README.md`.

## Naming convention

- types: `smlt_<name>_t` (`smlt_stage_t`, `smlt_ui_widget_t` for
  `smlt::ui::Widget`)
- functions/methods: `smlt_<type>_<method>(...)`, snake_case
  (`smlt_stage_aabb`)
- constructors: `smlt_<type>_create(...)`; overloads get a numeric suffix
  (`smlt_color_create`, `smlt_color_create2`, ...) ordered deterministically
  by parameter count then parameter type
- destructor: `smlt_<type>_destroy(smlt_<type>_t* self)`; for `smlt::Asset`
  subclasses (`Texture`, `Material`, ...) it's `smlt_<type>_release(...)`
  instead, since these are reference-counted and this drops one reference
  rather than unconditionally destroying the object -- see "Assets" in
  `ports/c/README.md`
- enum values: `SMLT_<VALUE>` if the C++ enumerator is already prefixed
  with the enum's name (`KEYBOARD_CODE_A` -> `SMLT_KEYBOARD_CODE_A`),
  otherwise `SMLT_<ENUM>_<VALUE>`
- fields: `smlt_<type>_get_<field>` / `_set_<field>`
- operators: a small curated set maps to named functions
  (`==`/`!=`/`<`/`<=`/`>`/`>=` -> `equals`/`not_equals`/`less_than`/...,
  `+`/`-`/`*`/`/` -> `add`/`subtract`/`multiply`/`divide`, unary `-` ->
  `negated`). Anything else (`[]`, `()`, `->`, `=`, `++`, ...) is skipped.

All C++ types are wrapped as opaque handles (`smlt_x_t*`), never as
structs with mirrored layout -- this keeps the C ABI stable even if a C++
class's private layout changes, at the cost of an allocation for anything
returned by value.

## Usage

```sh
pip install libclang   # provides the clang.cindex Python bindings
python3 tools/cgen/cgen.py            # regenerate with default settings
python3 tools/cgen/cgen.py -v         # verbose (info-level logging)
python3 tools/cgen/cgen.py -vv        # debug (also dumps clang args, unchanged-file skips)
python3 tools/cgen/cgen.py -q         # quiet (errors only)
python3 tools/cgen/cgen.py --strict   # exit non-zero if anything had to be skipped
python3 tools/cgen/cgen.py --no-vapi  # skip writing ports/vala/simulant-c.vapi
python3 tools/cgen/cgen.py --vapi /tmp/out.vapi --vapi-namespace Foo  # write it elsewhere / under a different namespace
```

Or via CMake, after configuring with `-DSIMULANT_BUILD_C_BINDINGS=ON`:

```sh
cmake --build . --target simulant-c-regenerate   # re-run cgen.py
cmake .                                          # re-run CMake if files were added/removed
cmake --build . --target simulant-c
```

By default `cgen.py` scans `simulant/simulant.h`, the umbrella header that
pulls in essentially the whole public API: `Application`, `Window`,
`Scene`/`SceneManager`, every stage node (`Actor`, `Camera`, `Light`,
`Sprite`, `Mesh`, UI widgets, ...), the math/asset/material types, and so
on (~350 classes as of this writing). SDL2's include path is discovered
automatically via `pkg-config`/`sdl2-config`; other third-party deps
pulled in transitively are listed in `DEFAULT_EXTRA_INCLUDE_DIRS` in
`cgen_lib/config.py`. Narrow the scan with `--input` if you only care
about a subset, e.g. `--input simulant/stage.h`.

Anything the scanner can't map to a consistent C signature is skipped
with a warning, not a hard failure (see below) -- expect several hundred
warnings scanning the full engine; that's the tool doing its job, not a
sign something is broken. A handful of classes are force-skipped via
`tools/cgen/ignore.json` because a header they (or one they transitively
include) pull in currently fails to compile under newer GCC's
`-Wtemplate-body` -- a pre-existing bug in that header, unrelated to the
generated bindings; see the comment in `ignore.json`.

## What gets skipped (and why)

Logged as a warning, with the reason and the fully-qualified C++ name:

- **class templates** and **explicit/partial template specializations** --
  there's no single concrete type to generate a handle for
- **function/method templates** -- same reason
- parameters or return types that are **STL containers**
  (`std::vector`, `std::pair`, `std::map`, ...), `std::function`,
  `std::unique_ptr`, C arrays, or anything else outside {primitives,
  `std::string`, enums in `smlt`, classes/structs in `smlt`,
  `std::shared_ptr<X>` where `X` derives from `smlt::Asset` -- see below}
- **rvalue reference** parameters/returns
- **variadic** functions
- most **operator overloads** (see the curated list above)
- a by-value parameter/return of a type with a **deleted copy
  constructor**
- classes that are **abstract** (have pure virtual methods) still get
  their methods wrapped (useful once you have a pointer from elsewhere),
  but no `_create()`
- classes with an **inaccessible or deleted destructor** get neither
  `_create()` nor `_destroy()` (there'd be no way to release them safely)
- classes with a **manager-owned lifetime** (anything deriving from
  `smlt::StageNode` or `smlt::Asset` -- `Actor`, `Camera`, ..., `Texture`,
  `Material`, `Mesh`, `Sound`, `Font`, ...) never get a mechanical
  `_create()` -- a raw `new` would compile but produce an object the
  owning manager never tracks or updates. `StageNode` subclasses don't get
  a mechanical `_destroy()` either (see `ports/c/README.md` for how
  they're actually created from C: `smlt_stage_node_create_custom`, in
  `stage_node_ext.h`). `Asset` subclasses *do* get a generated cleanup
  function (and every other method/field), just named `_release()`
  instead of `_destroy()` and routed through a `shared_ptr`-aware handle
  convention instead of a raw pointer -- see "Assets" in
  `ports/c/README.md` for the details and how to actually obtain one
  (`AssetManager::load_texture()` et al. are wrapped normally now)

Anything referenced only as a pointer/reference (e.g. a forward-declared
`Scene*` parameter when `scene.h` isn't part of the current scan) still
gets an opaque typedef so it can be passed around, even without full
method bindings.

Use `tools/cgen/ignore.json` to force-skip specific fully-qualified names
(classes, methods, constructors, free functions) that the heuristics above
don't catch on their own.

## Renaming generated names

Overloads beyond the first get a numeric suffix (`smlt_color_create2`,
`smlt_color_create3`, ...) since that's all cgen.py can derive mechanically
-- there's no reliable way to turn "takes a `const float*` and a count"
into a good name without knowing what the array actually represents.
`tools/cgen/renames.json` maps a *default* generated name to a
hand-chosen one:

```json
{
    "renames": {
        "smlt_color_create2": "smlt_color_create_from_array"
    }
}
```

This is applied as a post-processing pass after scanning, so it affects
both the C output and `--vapi` (a renamed overload becomes a nicer named
Vala constructor too -- see `ports/vala/README.md`). cgen.py validates the
file on every run: a key that doesn't match any generated name is a
stale entry (logged as a warning, generation continues), and a rename
that would collide with another symbol's name is skipped (also warned,
both symbols keep their original names). Keep the same `smlt_<base>_`
prefix as the original when choosing a replacement -- only the suffix is
meant to change; the prefix is what both the C output and (for
constructors specifically) the Vala emitter's named-constructor-suffix
derivation key off of.

**Caveat**: keys are the literal default name, which encodes cgen.py's
overload *ordering* (by parameter count, then parameter types -- see
above), not the overload's identity. If the underlying C++ overload set
changes, the numeric suffix a given overload gets can shift; the stale-key
warning catches a rename that now matches *nothing*, but can't catch one
that now silently matches a *different* overload than the one it was
written for. Re-check entries here after changing a constructor/method's
overload set.

## Known limitations

- Only **directly declared** public members are wrapped -- inherited
  public methods from base classes are not flattened into the derived
  class's bindings.
- By-value returns/params of a wrapped class allocate a heap copy; the
  caller owns it and must call the matching `_destroy()`. Returned
  `char*` strings must be released with `smlt_c_free_string()`.
- Pointer/reference returns are always treated as *borrowed* -- never
  call `_destroy()` on those.
- Two C++ overloads that map to the identical C signature (e.g. a
  `const char*` ctor and a `const std::string&` ctor) both get generated
  as separate, functionally-identical C functions; this is harmless but
  redundant.
- A class with **no user-declared constructor at all** gets no `_create()`,
  even though C++ would normally give it an implicit default one. Whether
  that implicit constructor actually exists depends on base classes and
  member initializers in ways libclang doesn't expose a direct query for;
  an earlier version of this tool guessed "yes" whenever no constructor
  was declared, which produced real compile failures for classes where
  the guess was wrong. If you need one of these constructed from C, either
  add an explicit constructor in C++, or hand-verify it and add a small
  helper next to the relevant `ports/c/include/simulant/c/*_ext.h` (see
  `smlt_app_config_create_default()` for a worked example).
