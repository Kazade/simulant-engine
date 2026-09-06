"""Small helpers on top of libclang's cindex that don't belong to any single
stage of the pipeline.
"""
import functools
import os
import shutil
import subprocess

from clang.cindex import CursorKind


def qualified_name(cursor) -> str:
    parts = []
    c = cursor
    while c is not None and c.kind != CursorKind.TRANSLATION_UNIT:
        if c.spelling:
            parts.append(c.spelling)
        c = c.semantic_parent
    return "::".join(reversed(parts))


def namespace_parts_and_chain(cursor):
    """Walks up from `cursor` (a record or enum decl) splitting the chain of
    semantic parents into namespace components (e.g. ['smlt', 'ui']) and the
    chain of enclosing record names (e.g. ['Widget'] for a nested class
    Widget::Style). Returns (namespace_parts, record_chain) both in
    outer-to-inner order, not including `cursor` itself.
    """
    namespaces = []
    records = []
    c = cursor.semantic_parent
    while c is not None and c.kind != CursorKind.TRANSLATION_UNIT:
        if c.kind == CursorKind.NAMESPACE:
            namespaces.append(c.spelling)
        elif c.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
            records.append(c.spelling)
        c = c.semantic_parent
    namespaces.reverse()
    records.reverse()
    return namespaces, records


def is_derived_from(cursor, target_qualified_name: str, _cache=None) -> bool:
    """True if the class/struct definition `cursor` inherits (directly or
    transitively) from `target_qualified_name`, e.g. is_derived_from(actor_cursor,
    "smlt::StageNode"). `cursor` must be a definition (base specifiers
    aren't visible on a forward declaration).

    `_cache` is keyed by (class, target) -- a class's answer for one target
    base says nothing about its answer for a different target, so those
    must not share a cache slot.
    """
    if _cache is None:
        _cache = {}
    qn = qualified_name(cursor)
    key = (qn, target_qualified_name)
    if key in _cache:
        return _cache[key]
    _cache[key] = False  # break cycles before recursing
    for child in cursor.get_children():
        if child.kind != CursorKind.CXX_BASE_SPECIFIER:
            continue
        base_decl = child.type.get_declaration()
        if base_decl is None:
            continue
        if qualified_name(base_decl) == target_qualified_name:
            _cache[key] = True
            return True
        base_def = base_decl.get_definition()
        if base_def is not None and is_derived_from(base_def, target_qualified_name, _cache):
            _cache[key] = True
            return True
    return False


def is_inside_namespace(cursor, name: str) -> bool:
    c = cursor.semantic_parent
    while c is not None and c.kind != CursorKind.TRANSLATION_UNIT:
        if c.kind == CursorKind.NAMESPACE and c.spelling == name:
            return True
        c = c.semantic_parent
    return False


def is_inside_template(cursor) -> bool:
    c = cursor
    while c is not None and c.kind != CursorKind.TRANSLATION_UNIT:
        if c.kind in (CursorKind.CLASS_TEMPLATE, CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION,
                      CursorKind.FUNCTION_TEMPLATE):
            return True
        c = c.semantic_parent
    return False


@functools.lru_cache(maxsize=1)
def sdl2_include_dirs():
    """Simulant's windowing backend headers pull in SDL.h; discover its
    include path the same way the CMake build falls back to (pkg-config),
    so scanning simulant.h doesn't need it spelled out with -I by hand on
    every machine.
    """
    for cmd in (["pkg-config", "--cflags", "sdl2"], ["sdl2-config", "--cflags"]):
        try:
            out = subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL)
        except (subprocess.CalledProcessError, OSError):
            continue
        return [tok[2:] for tok in out.split() if tok.startswith("-I")]
    return []


@functools.lru_cache(maxsize=1)
def find_system_libclang():
    """Prefer the libclang.so belonging to the system `clang` over whatever
    a `pip install libclang` happens to bundle: they need to agree on which
    libstdc++/GCC builtins are understood, or parsing typical C++20 STL
    headers can fail with spurious errors (e.g. a much older bundled
    libclang choking on a newer libstdc++).
    """
    resource_dir = clang_resource_dir()
    candidates = []
    if resource_dir:
        # resource dir looks like .../lib/clang/22, the matching libclang is
        # usually a couple of directories up, in .../lib or .../lib64.
        d = resource_dir
        for _ in range(4):
            d = os.path.dirname(d)
            candidates += [os.path.join(d, "libclang.so"), os.path.join(d, "lib64", "libclang.so")]
    try:
        out = subprocess.check_output(["ldconfig", "-p"], text=True)
        for line in out.splitlines():
            if "libclang.so" in line and "=>" in line:
                candidates.append(line.split("=>")[-1].strip())
    except (subprocess.CalledProcessError, OSError):
        pass
    for c in candidates:
        if c and os.path.exists(c):
            return c
    return None


@functools.lru_cache(maxsize=1)
def clang_resource_dir():
    """libclang's bundled headers (stddef.h etc) can mismatch the system
    clang's headers used elsewhere in the build. We ask the `clang` binary
    on PATH for its resource dir and pass it explicitly with -resource-dir
    so parsing doesn't spuriously fail to find builtin headers.
    """
    clang_bin = shutil.which("clang") or shutil.which("clang++")
    if not clang_bin:
        return None
    try:
        out = subprocess.check_output([clang_bin, "-print-resource-dir"], text=True)
        return out.strip()
    except (subprocess.CalledProcessError, OSError):
        return None
