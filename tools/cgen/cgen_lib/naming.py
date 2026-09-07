"""Name-mangling helpers shared by the scanner and the emitter.

All generated identifiers follow the same convention:

  - types get a ``smlt_`` prefix and a ``_t`` suffix (``smlt_stage_t``)
  - functions get a ``smlt_`` prefix (``smlt_stage_create``)
  - everything is snake_case, including things that were already
    snake_case in the C++ source (so the conversion is idempotent)
"""
import re

_CAMEL_RE1 = re.compile(r"(.)([A-Z][a-z]+)")
_CAMEL_RE2 = re.compile(r"([a-z0-9])([A-Z])")
_NON_ALNUM_RE = re.compile(r"[^0-9a-zA-Z]+")

# A digit run immediately followed by a single trailing uppercase letter
# that *isn't* the start of a new word (e.g. "2D"/"3D" in Camera3D,
# Horde3D) reads as one unit ("2d"/"3d"), not two. Left to _CAMEL_RE2
# alone, "Camera3D" would split as "camera3_d" (between the digit and the
# letter) instead of "camera_3d" (before the digit) -- this runs first and
# lowercases the trailing letter so _CAMEL_RE2 has nothing left to match
# there. Doesn't fire for a trailing digit with no letter after it (Vec3
# stays "vec3") or for a digit preceded by an uppercase letter, i.e. an
# all-caps acronym rather than a real word (MD2Loader, MS3D -- handled
# separately below).
_DIMENSION_SUFFIX_RE = re.compile(r"([a-z])(\d+)([A-Z])(?![a-z])")

# The all-caps-acronym counterpart of the rule above: an uppercase run,
# then digits, then another uppercase run, with the whole thing *not*
# leading into a new Titlecase word (MS3D, GL2X, S3TC read as one token
# each: "ms3d", "gl2x", "s3tc"). The trailing `[A-Z]+` is written greedy
# on purpose: for "MS3DLoader" it first grabs "DL", the lookahead then
# rejects that (a real word, "Loader", follows), so the engine backs off
# to just "D" -- leaving "Loader" for _CAMEL_RE1 to split off normally,
# giving "ms3d_loader". For "MD2Loader" this never matches at all (the
# only possible trailing-run candidate is "L", which is followed by
# lowercase "oader" and fails the same lookahead with no shorter
# alternative to back off to), so it's untouched and still splits via
# _CAMEL_RE1 as before.
_ACRONYM_DIGIT_RE = re.compile(r"([A-Z]+)(\d+)([A-Z]+)(?![a-z])")


def snake_case(identifier: str) -> str:
    """Convert CamelCase, PascalCase or already-snake_case identifiers to
    snake_case. Idempotent: snake_case(snake_case(x)) == snake_case(x).
    """
    s = _NON_ALNUM_RE.sub("_", identifier)
    s = _ACRONYM_DIGIT_RE.sub(lambda m: (m.group(1) + m.group(2) + m.group(3)).lower(), s)
    s = _DIMENSION_SUFFIX_RE.sub(lambda m: f"{m.group(1)}_{m.group(2)}{m.group(3).lower()}", s)
    s = _CAMEL_RE1.sub(r"\1_\2", s)
    s = _CAMEL_RE2.sub(r"\1_\2", s)
    s = re.sub(r"_+", "_", s)
    return s.strip("_").lower()


def namespace_path_to_prefix(namespace_parts):
    """['smlt', 'ui'] -> 'ui' (the leading 'smlt' is implicit in SMLT_PREFIX)"""
    parts = [p for p in namespace_parts if p != "smlt"]
    return "_".join(snake_case(p) for p in parts)


SMLT_PREFIX = "smlt"


def c_type_base_name(namespace_parts, class_name_chain):
    """Builds the 'base name' used for a wrapped type, e.g.

    namespace_parts=['smlt', 'ui'], class_name_chain=['Widget', 'Style']
    -> 'ui_widget_style'
    """
    ns_prefix = namespace_path_to_prefix(namespace_parts)
    chain = "_".join(snake_case(c) for c in class_name_chain)
    if ns_prefix:
        return f"{ns_prefix}_{chain}"
    return chain


def c_type_name(base_name: str) -> str:
    return f"{SMLT_PREFIX}_{base_name}_t"


def c_func_name(base_name: str, method_name: str) -> str:
    return f"{SMLT_PREFIX}_{base_name}_{snake_case(method_name)}"


def c_free_func_name(namespace_parts, func_name: str) -> str:
    ns_prefix = namespace_path_to_prefix(namespace_parts)
    if ns_prefix:
        return f"{SMLT_PREFIX}_{ns_prefix}_{snake_case(func_name)}"
    return f"{SMLT_PREFIX}_{snake_case(func_name)}"


def c_enum_value_name(enum_base_name: str, enumerator_spelling: str) -> str:
    """Enumerators in Simulant are already conventionally prefixed
    (KEYBOARD_CODE_A), so we just add the SMLT_ prefix. If they're not
    prefixed at all we fall back to <ENUM>_<VALUE>.
    """
    upper_base = snake_case(enum_base_name).upper()
    if enumerator_spelling.upper().startswith(upper_base):
        return f"{SMLT_PREFIX.upper()}_{enumerator_spelling.upper()}"
    return f"{SMLT_PREFIX.upper()}_{upper_base}_{enumerator_spelling.upper()}"


def disambiguate(names):
    """Given a list of proposed identical names for overloads, returns a
    list of unique names: the first keeps the bare name, subsequent ones
    get a numeric suffix (_2, _3, ...).
    """
    result = []
    for i, _ in enumerate(names):
        result.append(names[i] if i == 0 else f"{names[i]}{i + 1}")
    return result
