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


def snake_case(identifier: str) -> str:
    """Convert CamelCase, PascalCase or already-snake_case identifiers to
    snake_case. Idempotent: snake_case(snake_case(x)) == snake_case(x).
    """
    s = _NON_ALNUM_RE.sub("_", identifier)
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
