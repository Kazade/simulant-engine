"""Renders a TranslationUnit (see ir.py) -- the same IR the C emitter
consumes -- to a Vala .vapi file describing libsimulant-c to the Vala
compiler. Reusing the IR (rather than re-parsing the generated C headers)
guarantees the vapi always matches the actual C symbols exactly.

Vala/GLib background this mapping leans on:
  - `[Compact]` classes are plain structs with manual memory management --
    exactly our opaque-handle model. `free_function` on the class makes
    Vala call it automatically when a value goes out of scope.
  - A method's default C calling convention passes the instance as the
    *first* argument, which is exactly our `self` convention, so instance
    methods only need `self` dropped from the Vala-visible parameter list.
  - A method returning a class type is `owned` (caller frees it) by
    default in Vala; add `unowned` to mark a borrowed reference. This
    lines up exactly with our `needs_free_doc` flag.
"""
import re

from . import log
from .ir import TranslationUnit

VALA_KEYWORDS = {
    "abstract", "as", "async", "base", "bool", "break", "case", "catch", "char",
    "checked", "class", "const", "construct", "continue", "default", "delegate",
    "delete", "do", "double", "dynamic", "else", "ensures", "enum", "errordomain",
    "extern", "false", "finally", "float", "for", "foreach", "get", "if", "in",
    "inline", "int", "int8", "int16", "int32", "int64", "interface", "internal",
    "is", "lock", "long", "namespace", "new", "null", "out", "override", "owned",
    "params", "partial", "private", "protected", "public", "ref", "requires",
    "return", "sealed", "set", "short", "signal", "sizeof", "static", "string",
    "struct", "switch", "this", "throw", "throws", "true", "try", "typeof",
    "uchar", "uint", "uint8", "uint16", "uint32", "uint64", "ulong", "unichar",
    "unowned", "unsafe", "ushort", "using", "value", "var", "virtual", "void",
    "volatile", "weak", "while", "yield",
}

PRIMITIVE_MAP = {
    "void": "void", "bool": "bool",
    "signed char": "int8", "unsigned char": "uint8",
    "short": "int16", "unsigned short": "uint16",
    "int": "int", "unsigned int": "uint",
    "long": "long", "unsigned long": "ulong",
    "long long": "int64", "unsigned long long": "uint64",
    "float": "float", "double": "double", "long double": "double",
    "size_t": "size_t", "ssize_t": "ssize_t",
    "int8_t": "int8", "uint8_t": "uint8",
    "int16_t": "int16", "uint16_t": "uint16",
    "int32_t": "int32", "uint32_t": "uint32",
    "int64_t": "int64", "uint64_t": "uint64",
    "wchar_t": "unichar",
}


def _vala_identifier(name: str) -> str:
    return f"@{name}" if name in VALA_KEYWORDS else name


def _pascal_case(base_name: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in base_name.split("_") if part)


def _strip_prefix(c_name: str, prefix: str) -> str:
    if c_name.startswith(prefix):
        return c_name[len(prefix):]
    return c_name  # shouldn't happen: every c_name we generated has this prefix


def _parse_c_type(c_type: str):
    """('const smlt_x_t*', ...) -> (base_identifier, is_pointer, is_const)"""
    s = c_type.strip()
    is_const = s.startswith("const ")
    if is_const:
        s = s[len("const "):].strip()
    is_pointer = s.endswith("*")
    if is_pointer:
        s = s[:-1].strip()
    return s, is_pointer, is_const


class VapiEmitter:
    def __init__(self, namespace="Smlt", library="simulant-c", cheader="simulant/c/simulant_c.h"):
        self.namespace = namespace
        self.library = library
        self.cheader = cheader
        self.class_names = {}   # c_type -> Vala class name
        self.enum_names = {}    # c_type -> Vala enum name
        self.int_aliases = set()  # c_type known to just be a plain int typedef

    # ---------------------------------------------------------------- setup
    def _build_index(self, tu: TranslationUnit):
        for cls in tu.classes:
            self.class_names[cls.c_type] = _pascal_case(cls.base_name)
        for op in tu.opaque_types:
            self.class_names[op.c_type] = _pascal_case(op.base_name)
        for enum in tu.enums:
            self.enum_names[enum.c_type] = _pascal_case(enum.base_name)
        for op in tu.opaque_enum_types:
            self.int_aliases.add(op.c_type)

    # ---------------------------------------------------------------- types
    def _vala_type(self, c_type: str, *, owned: bool, is_return: bool) -> str:
        base, is_pointer, is_const = _parse_c_type(c_type)

        if base == "char" and is_pointer:
            if not is_return:
                # `string` params are unowned/borrowed by default in Vala
                # already -- writing `unowned` explicitly just produces a
                # "redundant" warning from valac.
                return "string"
            # `unowned string` for *returns* unconditionally, even when
            # needs_free_doc says we own it: our owned string returns come
            # from smlt_c_strdup(), which uses plain malloc(), not GLib's
            # allocator -- letting Vala's default `owned string` semantics
            # free it with g_free() would be a malloc/g_free mismatch.
            # Callers of a method documented as returning an owned string
            # must free it explicitly with Smlt.free_string() (see the
            # fixed preamble below).
            return "unowned string"

        if base == "void":
            return "void*" if is_pointer else "void"

        if base in PRIMITIVE_MAP:
            vala = PRIMITIVE_MAP[base]
            return f"{vala}*" if is_pointer else vala

        if base in self.int_aliases:
            return "int"

        if base in self.enum_names:
            return self.enum_names[base]

        if base in self.class_names:
            name = self.class_names[base]
            if is_return and not owned:
                return f"unowned {name}"
            return name

        log.warning(f"vapi: no Vala mapping for C type '{c_type}', falling back to void*")
        return "void*"

    def _vala_param_type(self, c_type: str) -> str:
        return self._vala_type(c_type, owned=False, is_return=False)

    def _vala_return_type(self, return_spec) -> str:
        return self._vala_type(return_spec.c_type, owned=return_spec.needs_free_doc, is_return=True)

    def _vala_params(self, params, skip_self: bool) -> str:
        items = params[1:] if skip_self else params
        return ", ".join(f"{self._vala_param_type(p.c_type)} {_vala_identifier(p.name)}" for p in items)

    # ---------------------------------------------------------------- enums
    def _emit_enum(self, enum, out):
        vala_name = self.enum_names[enum.c_type]
        out.append(f'    [CCode (cname = "{enum.c_type}", cprefix = "", has_type_id = false)]')
        out.append(f"    public enum {vala_name} {{")
        for value in enum.enumerators:
            member = _vala_identifier(value.cpp_name)
            out.append(f'        [CCode (cname = "{value.c_name}")]')
            out.append(f"        {member},")
        out.append("    }")
        out.append("")

    # ---------------------------------------------------------------- classes
    def _emit_class(self, cls, out):
        vala_name = self.class_names[cls.c_type]
        destroy = next((m.c_name for m in cls.methods if m.is_destructor), None)
        ctors = [m for m in cls.methods if m.is_constructor]
        regular = [m for m in cls.methods if not m.is_constructor and not m.is_destructor]

        attrs = [f'cname = "{cls.c_type}"', "has_type_id = false"]
        if destroy:
            attrs.append(f'free_function = "{destroy}"')
        out.append(f"    [CCode ({', '.join(attrs)})]")
        out.append(f"    [Compact]")
        out.append(f"    public class {vala_name} {{")

        # Vala names the primary constructor after the class; overloads
        # beyond the first become named constructors. The default
        # (create2, create3, ...) numeric suffix becomes .overload2,
        # .overload3, ...; a hand-picked name from renames.json (e.g.
        # smlt_color_create_from_array) instead becomes .from_array,
        # since it already reads fine as a Vala constructor name without
        # the "overload" filler word.
        for i, ctor in enumerate(ctors):
            suffix = _strip_prefix(ctor.c_name, f"smlt_{cls.base_name}_create")
            if not suffix:
                ctor_name = vala_name
            elif suffix.isdigit():
                ctor_name = f"{vala_name}.overload{suffix}"
            else:
                ctor_name = f"{vala_name}.{suffix.lstrip('_')}"
            out.append(f'        [CCode (cname = "{ctor.c_name}")]')
            out.append(f"        public {ctor_name}({self._vala_params(ctor.params, skip_self=False)});")

        for m in regular:
            self._emit_method(m, cls.base_name, out, is_static=m.is_static)

        out.append("    }")
        out.append("")

    def _emit_method(self, m, base_name, out, is_static):
        vala_method = _vala_identifier(_strip_prefix(m.c_name, f"smlt_{base_name}_"))
        ret = self._vala_return_type(m.return_spec)
        params = self._vala_params(m.params, skip_self=not is_static)
        modifier = "static " if is_static else ""
        if m.return_spec.needs_free_doc and "unowned string" not in ret:
            out.append(f"        /** Caller owns the result; free with the matching release/destroy. */")
        if m.return_spec.c_type == "char*":
            out.append(f"        /** Caller owns the returned string; free it with Smlt.free_string(). */")
        out.append(f'        [CCode (cname = "{m.c_name}")]')
        out.append(f"        public {modifier}extern {ret} {vala_method}({params});")

    # ---------------------------------------------------------------- opaque-only
    def _emit_opaque_class(self, op, out):
        vala_name = self.class_names[op.c_type]
        out.append(f'    [CCode (cname = "{op.c_type}", has_type_id = false)]')
        out.append(f"    [Compact]")
        out.append(f"    /* {op.cpp_qualified_name}: referenced only as a pointer, not fully bound. */")
        out.append(f"    public class {vala_name} {{")
        out.append("    }")
        out.append("")

    # ---------------------------------------------------------------- free functions
    def _emit_free_function(self, f, out):
        vala_name = _vala_identifier(_strip_prefix(f.c_name, "smlt_"))
        ret = self._vala_return_type(f.return_spec)
        params = self._vala_params(f.params, skip_self=False)
        out.append(f'    [CCode (cname = "{f.c_name}")]')
        out.append(f"    public static extern {ret} {vala_name}({params});")
        out.append("")

    # ---------------------------------------------------------------- entry point
    def write(self, tu: TranslationUnit, out_path: str):
        self._build_index(tu)

        out = []
        out.append("/* Generated by tools/cgen/cgen.py -- do not edit by hand.")
        out.append(" * Regenerate with: python3 tools/cgen/cgen.py --vapi <path>")
        out.append(" */")
        out.append(f'[CCode (cheader_filename = "{self.cheader}")]')
        out.append(f"namespace {self.namespace} {{")
        out.append("")
        out.append("    /* Releases a string returned by any method documented as")
        out.append("     * transferring ownership of its string result (see the doc")
        out.append("     * comments above such methods below). Not GLib-allocated --")
        out.append("     * do not use g_free()/free the value returned by any other")
        out.append("     * method that returns `string` without such a comment. */")
        out.append('    [CCode (cname = "smlt_c_free_string")]')
        out.append("    public static extern void free_string(void* str);")
        out.append("")

        for enum in sorted(tu.enums, key=lambda e: e.c_type):
            self._emit_enum(enum, out)
        for cls in sorted(tu.classes, key=lambda c: c.c_type):
            self._emit_class(cls, out)
        for op in sorted(tu.opaque_types, key=lambda o: o.c_type):
            self._emit_opaque_class(op, out)
        for f in tu.functions:
            self._emit_free_function(f, out)

        out.append("}")

        content = "\n".join(out) + "\n"
        with open(out_path, "w") as fh:
            fh.write(content)
        log.status(f"generated Vala bindings ({len(tu.classes)} classes, {len(tu.enums)} enums, "
                   f"{len(tu.functions)} free functions) -> {out_path}")
