"""Applies user-supplied renames (tools/cgen/renames.json) to the IR after
scanning, so both the C and Vala emitters see the renamed symbols. Kept as
a separate post-processing pass rather than threaded through the scanner
because it's a pure name substitution over already-finalized c_names --
nothing about *how* a name was derived (overload disambiguation, field
accessor, operator mapping, ...) matters here, only the final string.
"""
from . import log


def apply_renames(tu, rename_map):
    """Mutates every Method/FreeFunction c_name in `tu` that matches a key
    in `rename_map`. Logs a warning (and leaves it un-renamed) for any
    rename that would collide with another symbol's name, and a warning
    for any rename entry that matched nothing at all -- likely stale,
    e.g. the C++ source changed and the overload numbering shifted so a
    different overload now has that default name.
    """
    if not rename_map:
        return

    # (c_name, owner_desc) for every symbol, in scan order.
    entries = []
    for cls in tu.classes:
        for m in cls.methods:
            entries.append((m, f"{cls.cpp_qualified_name}::{m.cpp_name}"))
    for f in tu.functions:
        entries.append((f, f.cpp_qualified_name))

    # First pass: figure out the *proposed* final name for every symbol
    # without mutating anything yet, so a collision can be caught and
    # reported for both sides before either is renamed.
    matched_keys = {obj.c_name for obj, _ in entries if obj.c_name in rename_map}
    proposed = [rename_map.get(obj.c_name, obj.c_name) for obj, _ in entries]
    owners_by_name = {}
    for (obj, owner_desc), name in zip(entries, proposed):
        owners_by_name.setdefault(name, []).append((obj, owner_desc))

    for name, owners in owners_by_name.items():
        if len(owners) > 1:
            descs = ", ".join(f"{desc} ({obj.c_name})" for obj, desc in owners)
            log.warning(f"renames.json: '{name}' would be produced by more than one "
                       f"symbol ({descs}); leaving all of them un-renamed")
            continue
        obj, owner_desc = owners[0]
        if obj.c_name in rename_map and rename_map[obj.c_name] == name:
            obj.c_name = name

    for key in rename_map:
        if key not in matched_keys:
            log.warning(f"renames.json: '{key}' doesn't match any generated name -- "
                       f"stale entry, or the overload numbering shifted since it was written")
