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

"Managed" classes (Application, Scene, SceneManager, StageNode and its
whole built-in hierarchy: Sprite, Camera2d, ...) get a third treatment,
different from both of the above: real Vala classes with real bodies,
written as actual `.vala` source (not `.vapi` extern declarations) to
ports/vala/generated/ -- see write_managed_classes() for why (virtual/
override support, and for StageNode specifically a wrap()-cache bridging
raw pointers to Vala object identity).

Every mechanical method on a managed class is auto-generated exactly like
any other class's -- the *only* hand-written part is a small "custom"
snippet per class (ports/vala/runtime/custom/<base_name>.vala, spliced
into the generated class body verbatim) holding whatever genuinely can't
be derived mechanically: virtual hook declarations, constructors,
destructors, and bespoke logic like StageNode.create_child<T>() or
SceneManager.register_scene<T>(). Adding a new managed class is meant to
be cheap: add its qualified name to HAND_WRITTEN_VALA_CLASSES below, and
only write a custom snippet if it actually needs one (a virtual hook, a
non-trivial constructor, ...) -- most of a class's surface should need no
snippet content at all.
"""
import os
import re

from . import log
from .ir import TranslationUnit

# Classes needing a real (non-[Compact]) generated Vala class -- see the
# module docstring above. Every OTHER class/function's references to one
# of these (as a parameter or return type) fall back to `void*`, since a
# plain [Compact]-style `extern` declaration can't marshal a real object
# automatically; see respect_stage_node in _vala_type().
#
# smlt::StageNode itself is deliberately *not* listed here: every class
# derived from it (is_stage_node_hierarchy) is already a managed class
# implicitly, StageNode included -- see _is_managed().
HAND_WRITTEN_VALA_CLASSES = {
    "smlt::Application",
    "smlt::Scene",
    "smlt::SceneManager",
}

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
    def __init__(self, namespace="Smlt", library="simulant-c", cheader="simulant/c/simulant_c.h",
                 excluded_classes=None):
        self.namespace = namespace
        self.library = library
        self.cheader = cheader
        self.excluded_classes = excluded_classes if excluded_classes is not None else HAND_WRITTEN_VALA_CLASSES
        self.class_names = {}   # c_type -> Vala class name
        self.enum_names = {}    # c_type -> Vala enum name
        self.int_aliases = set()  # c_type known to just be a plain int typedef
        self.managed_c_types = set()  # c_type of every "managed" class (see module docstring)
        self.stage_node_c_types = set()  # c_type of is_stage_node_hierarchy classes specifically
        self._stage_node_root_c_type = None  # c_type of smlt::StageNode itself
        self.qualified_to_vala = {}  # cpp_qualified_name -> Vala class name
        # base_name -> custom snippet text, loaded once in write_managed_classes()
        self._custom_snippets = {}
        # method/property short names reserved by some managed class's
        # `virtual`/`override` declarations -- see _reserved_names().
        self.__reserved_names = set()

    def _is_managed(self, cls) -> bool:
        return cls.is_stage_node_hierarchy or cls.cpp_qualified_name in self.excluded_classes

    # ---------------------------------------------------------------- setup
    def _build_index(self, tu: TranslationUnit):
        for cls in tu.classes:
            self.class_names[cls.c_type] = _pascal_case(cls.base_name)
            self.qualified_to_vala[cls.cpp_qualified_name] = _pascal_case(cls.base_name)
            if self._is_managed(cls):
                self.managed_c_types.add(cls.c_type)
            if cls.is_stage_node_hierarchy:
                self.stage_node_c_types.add(cls.c_type)
                if cls.cpp_qualified_name == "smlt::StageNode":
                    self._stage_node_root_c_type = cls.c_type
        for op in tu.opaque_types:
            self.class_names[op.c_type] = _pascal_case(op.base_name)
        for enum in tu.enums:
            self.enum_names[enum.c_type] = _pascal_case(enum.base_name)
        for op in tu.opaque_enum_types:
            self.int_aliases.add(op.c_type)

    # ---------------------------------------------------------------- types
    def _vala_type(self, c_type: str, *, owned: bool, is_return: bool, respect_stage_node: bool = True) -> str:
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

        if respect_stage_node and base in self.managed_c_types:
            # A managed type (see module docstring) referenced from an
            # *unrelated* ordinary class (e.g. SceneCompositor.create_layer
            # (StageNode, Camera, ...), or anything referencing Application/
            # Scene/SceneManager): these are real Vala objects, not
            # [Compact] structs, so a plain `extern` declaration can't
            # marshal them automatically. Falls back to the raw pointer --
            # callers pass `.native` and wrap results with StageNode.wrap()
            # (or the hand-written class's own equivalent) themselves.
            return "void*"

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

    def _vala_param_type(self, c_type: str, *, respect_stage_node: bool = True) -> str:
        return self._vala_type(c_type, owned=False, is_return=False, respect_stage_node=respect_stage_node)

    def _vala_return_type(self, return_spec, *, respect_stage_node: bool = True) -> str:
        return self._vala_type(return_spec.c_type, owned=return_spec.needs_free_doc, is_return=True,
                                respect_stage_node=respect_stage_node)

    def _vala_params(self, params, skip_self: bool) -> str:
        items = params[1:] if skip_self else params
        return ", ".join(f"{self._vala_param_type(p.c_type)} {_vala_identifier(p.name)}" for p in items)

    def _is_managed_type(self, c_type: str) -> bool:
        base, _, _ = _parse_c_type(c_type)
        return base in self.managed_c_types

    def _wrap_root_for(self, c_type_base: str):
        """Which class's wrap() should reconstitute a managed-type value
        returned as a raw void*, and whether the caller needs to cast the
        wrap() result back down to the concrete return type.

        StageNode-hierarchy types all share one wrapper cache rooted at
        StageNode itself (StageNode.wrap() then reports back the *actual*
        concrete type via node_type_name(), so a cast is needed whenever
        the declared return type is some subclass). Every other managed
        family (Application, Scene, SceneManager, ...) wraps at itself --
        there's exactly one concrete class in that family as of writing, so
        no cast is ever needed.
        """
        if c_type_base in self.stage_node_c_types:
            return "StageNode", c_type_base != self._stage_node_root_c_type
        return self.class_names[c_type_base], False

    def _extern_type(self, c_type: str, *, owned: bool, is_return: bool) -> str:
        """Like _vala_type, but a managed-class reference always becomes
        void* -- these classes are real Vala objects, not [Compact]
        structs, so they can't marshal across an extern boundary
        automatically the way every other wrapped type does. Used only
        for the low-level private extern declarations the public,
        wrap()-aware methods below call through.
        """
        if self._is_managed_type(c_type):
            return "void*"
        return self._vala_type(c_type, owned=owned, is_return=is_return, respect_stage_node=False)

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

    # ---------------------------------------------------------------- managed classes
    # Matches only `virtual`/`override` declarations -- reserved *globally*
    # (see _load_custom_snippets()), since these are inheritable across the
    # whole managed hierarchy (Sprite has no snippet of its own but must
    # still avoid colliding with StageNode's inherited virtual hooks).
    _VIRTUAL_METHOD_RE = re.compile(r"\bpublic\s+(?:static\s+)?(?:virtual|override)\b[^;{]*?\b(\w+)\s*(?:<[^>]*>)?\s*\(")
    # Matches any public method/constructor declaration -- reserved only
    # within the *same* class's own snippet (see _skip_names_for()), e.g.
    # StageNode's own hand-written create_child<T>() vs. the mechanical
    # (non-generic) create_child(string, Params) C++ also declares.
    _ANY_METHOD_RE = re.compile(r"\bpublic\s+(?:static\s+)?(?:virtual\s+|override\s+)?[\w<>?\[\],.\s]+?\b(\w+)\s*(?:<[^>]*>)?\s*\(")

    def _reserved_names(self):
        return self.__reserved_names

    def _skip_names_for(self, cls):
        """Method short names to skip mechanically generating for `cls`:
        globally-reserved virtual hook names, plus anything `cls`'s own
        custom snippet already declares under any name.
        """
        names = set(self.__reserved_names)
        snippet = self._custom_snippets.get(cls.base_name)
        if snippet:
            names.update(self._ANY_METHOD_RE.findall(snippet))
        return names

    def _load_custom_snippets(self, tu: TranslationUnit, custom_dir: str):
        """Reads ports/vala/runtime/custom/<base_name>.vala for every
        managed class that has one (optional -- most managed classes,
        e.g. any built-in node type like Sprite, need none at all).
        """
        self._custom_snippets = {}
        reserved = set()
        for cls in tu.classes:
            if not self._is_managed(cls):
                continue
            path = os.path.join(custom_dir, f"{cls.base_name}.vala")
            if not os.path.isfile(path):
                continue
            with open(path) as fh:
                text = fh.read()
            self._custom_snippets[cls.base_name] = text
            reserved.update(self._VIRTUAL_METHOD_RE.findall(text))
        self.__reserved_names = reserved

    def _direct_base_for(self, cls) -> str:
        if cls.is_stage_node_hierarchy and cls.cpp_qualified_name != "smlt::StageNode":
            return self.qualified_to_vala.get(cls.direct_base_qualified_name, "StageNode")
        return "GLib.Object"

    def _emit_managed_method(self, m, cls, out):
        base_name = cls.base_name
        raw_short_name = _strip_prefix(m.c_name, f"smlt_{base_name}_")
        short_name = _vala_identifier(raw_short_name)
        is_static = m.is_static
        params = m.params if is_static else m.params[1:]

        extern_param_decls = [] if is_static else ["void* self"]
        extern_args_for_public = []  # how the public method calls the extern
        public_params = []
        for p in params:
            extern_param_decls.append(f"{self._extern_type(p.c_type, owned=False, is_return=False)} {_vala_identifier(p.name)}")
            public_params.append(f"{self._vala_param_type(p.c_type, respect_stage_node=False)} {_vala_identifier(p.name)}")
            name = _vala_identifier(p.name)
            if self._is_managed_type(p.c_type):
                extern_args_for_public.append(f"({name} == null ? null : {name}.native)")
            else:
                extern_args_for_public.append(name)

        extern_ret = self._extern_type(m.return_spec.c_type, owned=m.return_spec.needs_free_doc, is_return=True)
        is_managed_return = self._is_managed_type(m.return_spec.c_type)
        if is_managed_return:
            # wrap()'s cache always hands back a properly ref-counted
            # owned reference regardless of whether the underlying C++
            # method borrows or (nominally) transfers -- managed types have
            # no free_function/traditional ownership at all (see
            # MANAGED_LIFETIME_BASES), so the mechanical owned/unowned
            # convention doesn't apply here.
            ret_base, _, _ = _parse_c_type(m.return_spec.c_type)
            public_ret = self.class_names[ret_base] + "?"
            wrap_root, wrap_needs_cast = self._wrap_root_for(ret_base)
        else:
            public_ret = self._vala_return_type(m.return_spec, respect_stage_node=False)

        extern_name = f"_c_{base_name}_{raw_short_name}"
        vala_cname = f"smlt_vala_{base_name}_{raw_short_name}"

        out.append(f'        [CCode (cname = "{m.c_name}", cheader_filename = "simulant/c/{base_name}.h")]')
        out.append(f"        private static extern {extern_ret} {extern_name}({', '.join(extern_param_decls)});")
        out.append("")
        if m.return_spec.needs_free_doc and "unowned string" not in public_ret:
            out.append(f"        /** Caller owns the result; free with the matching release/destroy. */")
        if m.return_spec.c_type == "char*":
            out.append(f"        /** Caller owns the returned string; free it with Smlt.free_string(). */")
        out.append(f'        [CCode (cname = "{vala_cname}")]')
        modifier = "static " if is_static else ""
        out.append(f"        public {modifier}{public_ret} {short_name}({', '.join(public_params)}) {{")
        call_args = extern_args_for_public if is_static else ["native"] + extern_args_for_public
        call_expr = f"{extern_name}({', '.join(call_args)})"
        if public_ret == "void":
            out.append(f"            {call_expr};")
        elif is_managed_return:
            wrapped = f"{wrap_root}.wrap({call_expr})"
            if wrap_needs_cast:
                # Cast target must be the bare class name -- "(unowned X)"/
                # "(X?)" aren't valid cast syntax; `unowned`/`?` only apply
                # to the enclosing method's own declared return type above.
                cast_type = public_ret.rstrip("?")
                if cast_type.startswith("unowned "):
                    cast_type = cast_type[len("unowned "):]
                wrapped = f"({cast_type}) {wrapped}"
            out.append(f"            return {wrapped};")
        else:
            out.append(f"            return {call_expr};")
        out.append("        }")
        out.append("")

    def _write_managed_class_file(self, cls, out_dir):
        vala_name = self.class_names[cls.c_type]
        direct_base = self._direct_base_for(cls)
        wants_factory = cls.is_stage_node_hierarchy and not cls.is_abstract and cls.cpp_qualified_name != "smlt::StageNode"

        out = []
        out.append("/* Generated by tools/cgen/cgen.py -- do not edit by hand.")
        out.append(" * Regenerate with: python3 tools/cgen/cgen.py")
        out.append(" *")
        out.append(f" * Mechanical methods only -- hand-written logic for this class (if any)")
        out.append(f" * lives in ports/vala/runtime/custom/{cls.base_name}.vala and is spliced in")
        out.append(f" * below verbatim. See the module docstring in vapi_emitter.py.")
        out.append(" */")
        out.append("using Smlt;")
        out.append("")
        out.append("namespace Smlt {")
        out.append(f'    [CCode (cheader_filename = "simulant-vala.h")]')
        out.append(f"    public class {vala_name} : {direct_base} {{")
        if direct_base == "GLib.Object":
            # Root of a managed class family (StageNode itself, or
            # Application/Scene/SceneManager, which have no interesting
            # Vala base beyond GLib.Object) -- subclasses inherit this
            # rather than redeclaring it. Public rather than internal/
            # private: needed by any *consumer* calling a mechanical
            # function elsewhere that references a managed type (see
            # respect_stage_node in _vala_type()), e.g.
            # SceneCompositor.create_layer(subtree.native, camera.native, ...).
            out.append("        public void* native;")
            out.append("")
        if wants_factory:
            # StageNode::create_child<T>() equivalent for this concrete
            # type (see emitter.py's _wants_create_child_factory) --
            # registered against typeof(this) in _stage_node_registry.vala
            # so StageNode.create_child<T>() can find it generically.
            out.append(f'        [CCode (cname = "smlt_stage_node_create_child_{cls.base_name}", '
                       f'cheader_filename = "simulant/c/{cls.base_name}.h")]')
            out.append("        internal static extern void* _create_native(void* parent);")
            out.append("")
        # Managed-lifetime classes (StageNode-derived) get no mechanical
        # constructor/destructor at all (see MANAGED_LIFETIME_BASES). A
        # class that does have one (Application's destructor, SceneManager's
        # constructor+destructor) needs a real hand-written wrapper -- never
        # generated -- but the snippet providing it still needs *some* way
        # to call the actual C++ constructor/destructor, so those are
        # exposed here as plain private externs the snippet calls directly.
        ctors = [m for m in cls.methods if m.is_constructor]
        dtor = next((m for m in cls.methods if m.is_destructor), None)
        for i, ctor in enumerate(ctors):
            suffix = "" if len(ctors) == 1 else f"_{i}"
            out.append(f'        [CCode (cname = "{ctor.c_name}", cheader_filename = "simulant/c/{cls.base_name}.h")]')
            out.append(f"        private static extern void* _c_create{suffix}({self._vala_params(ctor.params, skip_self=False)});")
            out.append("")
        if dtor:
            out.append(f'        [CCode (cname = "{dtor.c_name}", cheader_filename = "simulant/c/{cls.base_name}.h")]')
            out.append("        private static extern void _c_destroy(void* self);")
            out.append("")
        snippet = self._custom_snippets.get(cls.base_name)
        if snippet:
            out.append(snippet.rstrip("\n"))
            out.append("")
        reserved = self._skip_names_for(cls)
        for m in cls.methods:
            if m.is_constructor or m.is_destructor:
                continue
            short_name = _strip_prefix(m.c_name, f"smlt_{cls.base_name}_")
            if short_name in reserved:
                continue
            self._emit_managed_method(m, cls, out)
        out.append("    }")
        out.append("}")

        path = os.path.join(out_dir, f"{cls.base_name}.vala")
        with open(path, "w") as fh:
            fh.write("\n".join(out) + "\n")

    def _write_stage_node_registry(self, tu, out_dir):
        out = []
        out.append("/* Generated by tools/cgen/cgen.py -- do not edit by hand.")
        out.append(" * Regenerate with: python3 tools/cgen/cgen.py")
        out.append(" */")
        out.append("namespace Smlt {")
        out.append("    /* Registers every concrete built-in StageNode-derived class's")
        out.append("     * GType against its C++ node_type_name() -- see StageNode.wrap()")
        out.append("     * in ports/vala/runtime/custom/stage_node.vala. Assumes node_type_name()")
        out.append("     * matches the mechanical snake_case class name, which holds for")
        out.append("     * every class using S_DEFINE_STAGE_NODE_META's usual convention;")
        out.append("     * a class where that assumption doesn't hold just falls back to a")
        out.append("     * plain (non-polymorphic) StageNode wrapper when reached generically")
        out.append("     * -- safe, if imprecise, not a crash. */")
        out.append("    public class StageNodeRegistry {")
        out.append("        internal static void register_all() {")
        for cls in sorted(tu.classes, key=lambda c: c.c_type):
            if not cls.is_stage_node_hierarchy or cls.cpp_qualified_name == "smlt::StageNode":
                continue
            if cls.is_abstract:
                continue
            vala_name = self.class_names[cls.c_type]
            out.append(f'            StageNode.register_builtin_type("{cls.base_name}", typeof({vala_name}));')
            out.append(f"            StageNode.register_builtin_factory(typeof({vala_name}), {vala_name}._create_native);")
        out.append("        }")
        out.append("    }")
        out.append("}")

        path = os.path.join(out_dir, "_stage_node_registry.vala")
        with open(path, "w") as fh:
            fh.write("\n".join(out) + "\n")

    def write_managed_classes(self, tu: TranslationUnit, out_dir: str, custom_dir: str):
        """Call after write(): emits one real .vala source file per
        managed class (see module docstring) -- Application, Scene,
        SceneManager, StageNode, and every built-in node type -- with
        every mechanical method auto-generated and any hand-written logic
        from ports/vala/runtime/custom/<base_name>.vala spliced in
        verbatim, plus a small generated registry file for the built-in
        node type/factory lookup StageNode.wrap()/create_child<T>() use.
        Requires _build_index() to have already run (done by write()).
        """
        os.makedirs(out_dir, exist_ok=True)
        self._load_custom_snippets(tu, custom_dir)
        n = 0
        for cls in tu.classes:
            if self._is_managed(cls):
                self._write_managed_class_file(cls, out_dir)
                n += 1
        self._write_stage_node_registry(tu, out_dir)
        log.status(f"generated {n} managed Vala classes -> {out_dir}")

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
            if self._is_managed(cls):
                # Handled entirely separately -- see write_managed_classes().
                continue
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
