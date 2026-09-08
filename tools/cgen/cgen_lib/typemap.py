"""Maps clang.cindex.Type instances to the C types/expressions used in the
generated wrapper. Returns None (and logs a warning) for anything that can't
be represented consistently in a plain C API: STL containers, std::function,
raw C arrays, rvalue references, unions, and anything else that isn't either
a primitive, an enum in `smlt`, `std::string`, or a class/struct in `smlt`.
"""
from clang.cindex import CursorKind, TypeKind

from . import clangutil
from .ir import Param, ReturnSpec

PRIMITIVE_MAP = {
    TypeKind.BOOL: "bool",
    TypeKind.UCHAR: "unsigned char",
    TypeKind.CHAR_S: "char",
    TypeKind.SCHAR: "signed char",
    TypeKind.WCHAR: "wchar_t",
    TypeKind.USHORT: "unsigned short",
    TypeKind.SHORT: "short",
    TypeKind.UINT: "unsigned int",
    TypeKind.INT: "int",
    TypeKind.ULONG: "unsigned long",
    TypeKind.LONG: "long",
    TypeKind.ULONGLONG: "unsigned long long",
    TypeKind.LONGLONG: "long long",
    TypeKind.FLOAT: "float",
    TypeKind.DOUBLE: "double",
    TypeKind.LONGDOUBLE: "long double",
}

# Preferred spelling for typedef'd fixed-width/standard types: if the
# (sugared) type spelling matches one of these, we keep it verbatim instead
# of falling back to the canonical builtin, so the generated header reads
# naturally and only needs <stdint.h>/<stddef.h>.
PRESERVED_TYPEDEFS = {
    "size_t", "ssize_t",
    "int8_t", "uint8_t", "int16_t", "uint16_t",
    "int32_t", "uint32_t", "int64_t", "uint64_t",
}


def _normalize(spelling: str) -> str:
    return spelling.replace("const ", "").replace("volatile ", "").strip()


def _is_std_string(t) -> bool:
    # Checked against both sugared (`std::string`) and canonical spellings.
    # The canonical form expands the full template argument list (char
    # traits, allocator) and may or may not carry the __cxx11 inline
    # namespace depending on the active libstdc++ ABI, so this is a prefix
    # match rather than a fixed set of exact spellings.
    s = _normalize(t.spelling)
    if s in ("std::string", "std::__cxx11::string"):
        return True
    return s.startswith("std::__cxx11::basic_string<char") or s.startswith("std::basic_string<char")


class Unsupported(Exception):
    def __init__(self, reason):
        super().__init__(reason)
        self.reason = reason


class TypeMapper:
    def __init__(self, registry):
        self.registry = registry
        # Source files of every wrappable record this mapper has resolved a
        # type against since the last reset_tracking() call. A translation
        # unit that constructs `new smlt::Radians(...)` needs radians.h
        # included even if the enclosing class lives in degrees.h and only
        # forward-declares Radians -- this lets callers add exactly the
        # extra #includes a given class/function body actually needs.
        self.touched_source_files = set()

    def reset_tracking(self):
        self.touched_source_files = set()

    # -- classification -----------------------------------------------
    def _is_primitivelike(self, t) -> bool:
        """True for bool/int/float/... *and* typedefs to them (size_t,
        uint32_t, ...): libclang reports Type.kind as TypeKind.TYPEDEF for
        the latter, so classification must look through to the canonical
        type rather than switching on `t.kind` directly.
        """
        canon_kind = t.get_canonical().kind
        return canon_kind == TypeKind.BOOL or canon_kind in PRIMITIVE_MAP

    def _primitive_c_type(self, t):
        norm = _normalize(t.spelling)
        if norm in PRESERVED_TYPEDEFS:
            return norm
        canon_kind = t.get_canonical().kind
        if canon_kind == TypeKind.VOID:
            return "void"
        if canon_kind in PRIMITIVE_MAP:
            return PRIMITIVE_MAP[canon_kind]
        return None

    def _record_info(self, t):
        decl = t.get_declaration()
        if decl is None or decl.kind not in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
            return None
        if clangutil.is_inside_template(decl):
            return None
        qn = clangutil.qualified_name(decl)
        info = self.registry.lookup_record(qn)
        if info is not None:
            self.touched_source_files.add(info.source_file)
        return info, qn

    def _enum_info(self, t):
        decl = t.get_declaration()
        if decl is None or decl.kind != CursorKind.ENUM_DECL:
            return None
        qn = clangutil.qualified_name(decl)
        return self.registry.lookup_enum(qn), qn

    def _shared_ptr_element_type(self, t):
        """If `t` is (a typedef/reference to) std::shared_ptr<X>, returns
        X's Type; otherwise None. Only the canonical form exposes this --
        `t.kind` for a typedef like `TexturePtr` is TypeKind.TYPEDEF, and
        even the untypedef'd spelling reports TypeKind.UNEXPOSED until
        canonicalized.
        """
        canon = t.get_canonical()
        if canon.kind != TypeKind.RECORD or canon.get_num_template_arguments() != 1:
            return None
        decl = canon.get_declaration()
        if decl is None or clangutil.qualified_name(decl) != "std::shared_ptr":
            return None
        return canon.get_template_argument_type(0)

    def _shared_ptr_record(self, t):
        """Raises Unsupported if `t` is a shared_ptr but not one we can
        bind; returns None if `t` isn't a shared_ptr at all (letting the
        caller try other classifications); otherwise returns (info, qn)
        for the pointee, guaranteed to have uses_shared_ptr=True.
        """
        elem = self._shared_ptr_element_type(t)
        if elem is None:
            return None
        record_hit = self._record_info(elem)
        if record_hit is None:
            raise Unsupported(f"std::shared_ptr<{elem.spelling}> element type is outside the wrapped API")
        info, qn = record_hit
        if info is None:
            raise Unsupported(f"std::shared_ptr<{qn}> element type is outside the wrapped API")
        if not info.uses_shared_ptr:
            raise Unsupported(f"std::shared_ptr<{qn}> is only supported for smlt::Asset-derived types")
        return info, qn

    # -- parameters ------------------------------------------------------
    def bind_param(self, clang_type, name: str, ctx: str) -> Param:
        """Raises Unsupported(reason) if the type can't be bound."""
        t = clang_type
        spelling = t.spelling
        # A typedef'd pointer/reference (CameraPtr, StageNodePtr, ...) has
        # TypeKind.TYPEDEF here, not POINTER/LVALUEREFERENCE -- same story
        # as size_t/uint32_t needing canonicalization in _is_primitivelike.
        # Route on the canonical kind so these aren't missed entirely; the
        # pointee is likewise taken from the canonical type so it's fully
        # unwrapped too.
        canon_kind = t.get_canonical().kind

        if canon_kind == TypeKind.RVALUEREFERENCE:
            raise Unsupported(f"rvalue reference parameter '{name}' ({spelling})")

        if canon_kind == TypeKind.LVALUEREFERENCE:
            pointee = t.get_canonical().get_pointee()
            return self._bind_indirect_param(pointee, name, spelling, ctx, was_ref=True)

        if canon_kind == TypeKind.POINTER:
            pointee = t.get_canonical().get_pointee()
            return self._bind_indirect_param(pointee, name, spelling, ctx, was_ref=False)

        if canon_kind in (TypeKind.CONSTANTARRAY, TypeKind.INCOMPLETEARRAY, TypeKind.VARIABLEARRAY):
            raise Unsupported(f"array parameter '{name}' ({spelling})")

        if self._is_primitivelike(t):
            c_type = self._primitive_c_type(t)
            return Param(name=name, cpp_type_spelling=spelling, c_type=c_type, cpp_arg=name)

        if _is_std_string(t):
            return Param(name=name, cpp_type_spelling=spelling, c_type="const char*",
                         cpp_arg=f"std::string({name} ? {name} : \"\")")

        sp_hit = self._shared_ptr_record(t)
        if sp_hit is not None:
            info, qn = sp_hit
            return Param(name=name, cpp_type_spelling=spelling, c_type=f"{info.c_type}*",
                         cpp_arg=f"(*reinterpret_cast<std::shared_ptr<{qn}>*>({name}))")

        enum_hit = self._enum_info(t)
        if enum_hit is not None:
            info, qn = enum_hit
            if info is None:
                raise Unsupported(f"enum '{qn}' used by '{name}' is outside the wrapped API")
            return Param(name=name, cpp_type_spelling=spelling, c_type=info.c_type,
                         cpp_arg=f"static_cast<{qn}>({name})")

        record_hit = self._record_info(t)
        if record_hit is not None:
            info, qn = record_hit
            if info is None:
                raise Unsupported(f"type '{qn}' used by-value for '{name}' is outside the wrapped API")
            if not info.is_copyable:
                raise Unsupported(f"type '{qn}' used by-value for '{name}' has a deleted copy constructor")
            # By-value class parameters are passed as a const pointer and
            # dereferenced on the C++ side -- C has no notion of by-value
            # opaque structs whose layout it doesn't know.
            return Param(name=name, cpp_type_spelling=spelling, c_type=f"const {info.c_type}*",
                         cpp_arg=f"(*reinterpret_cast<const {qn}*>({name}))")

        raise Unsupported(f"unsupported parameter type '{spelling}' for '{name}'")

    def _bind_indirect_param(self, pointee, name, orig_spelling, ctx, was_ref):
        is_const = pointee.is_const_qualified()

        if pointee.kind == TypeKind.VOID:
            c_type = "const void*" if is_const else "void*"
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type=c_type, cpp_arg=name)

        if pointee.kind == TypeKind.CHAR_S and is_const:
            # const char* / const char& (rare) -- treat as a C string.
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type="const char*", cpp_arg=name)

        if _is_std_string(pointee):
            expr = f"std::string({name} ? {name} : \"\")"
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type="const char*", cpp_arg=expr)

        sp_hit = self._shared_ptr_record(pointee)
        if sp_hit is not None:
            info, qn = sp_hit
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type=f"{info.c_type}*",
                         cpp_arg=f"(*reinterpret_cast<std::shared_ptr<{qn}>*>({name}))")

        if self._is_primitivelike(pointee):
            base = self._primitive_c_type(pointee)
            c_type = f"const {base}*" if is_const else f"{base}*"
            if was_ref:
                deref = f"(*{name})"
            else:
                deref = name
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type=c_type, cpp_arg=deref)

        enum_hit = self._enum_info(pointee)
        if enum_hit is not None:
            info, qn = enum_hit
            if info is None:
                raise Unsupported(f"enum pointer/reference '{qn}' used by '{name}' is outside the wrapped API")
            c_type = f"const {info.c_type}*" if is_const else f"{info.c_type}*"
            cast_type = f"const {qn}*" if is_const else f"{qn}*"
            ptr_expr = f"reinterpret_cast<{cast_type}>({name})"
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type=c_type,
                         cpp_arg=f"(*{ptr_expr})" if was_ref else ptr_expr)

        record_hit = self._record_info(pointee)
        if record_hit is not None:
            info, qn = record_hit
            if info is None:
                raise Unsupported(f"type '{qn}' pointer/reference used by '{name}' is outside the wrapped API")
            c_type = f"const {info.c_type}*" if is_const else f"{info.c_type}*"
            cast_type = f"const {qn}*" if is_const else f"{qn}*"
            ptr_expr = f"reinterpret_cast<{cast_type}>({name})"
            return Param(name=name, cpp_type_spelling=orig_spelling, c_type=c_type,
                         cpp_arg=f"(*{ptr_expr})" if was_ref else ptr_expr)

        raise Unsupported(f"unsupported pointee type '{pointee.spelling}' for '{name}'")

    # -- return values -----------------------------------------------------
    def bind_return(self, clang_type, ctx: str) -> ReturnSpec:
        """Raises Unsupported(reason) if the type can't be bound."""
        t = clang_type
        spelling = t.spelling
        # See the matching comment in bind_param: route on the canonical
        # kind so a typedef'd pointer/reference (CameraPtr, LayerPtr, ...)
        # isn't missed just because Type.kind reports TYPEDEF for it.
        canon_kind = t.get_canonical().kind

        if canon_kind == TypeKind.VOID:
            return ReturnSpec(c_type="void", stmt_template="{call};")

        if canon_kind == TypeKind.RVALUEREFERENCE:
            raise Unsupported(f"function returns an rvalue reference ({spelling})")

        if canon_kind in (TypeKind.LVALUEREFERENCE, TypeKind.POINTER):
            pointee = t.get_canonical().get_pointee()
            return self._bind_indirect_return(pointee, spelling, was_pointer=(canon_kind == TypeKind.POINTER))

        if canon_kind in (TypeKind.CONSTANTARRAY, TypeKind.INCOMPLETEARRAY):
            raise Unsupported(f"function returns an array ({spelling})")

        if self._is_primitivelike(t):
            c_type = self._primitive_c_type(t)
            return ReturnSpec(c_type=c_type, stmt_template="return {call};")

        if _is_std_string(t):
            return ReturnSpec(c_type="char*", stmt_template="return smlt_c_strdup(({call}).c_str());",
                              needs_free_doc=True)

        sp_hit = self._shared_ptr_record(t)
        if sp_hit is not None:
            info, qn = sp_hit
            return self._shared_ptr_return_spec(info, qn)

        enum_hit = self._enum_info(t)
        if enum_hit is not None:
            info, qn = enum_hit
            if info is None:
                raise Unsupported(f"function returns enum '{qn}' which is outside the wrapped API")
            return ReturnSpec(c_type=info.c_type, stmt_template=f"return static_cast<{info.c_type}>({{call}});")

        record_hit = self._record_info(t)
        if record_hit is not None:
            info, qn = record_hit
            if info is None:
                raise Unsupported(f"function returns '{qn}' by value which is outside the wrapped API")
            if not info.is_copyable:
                raise Unsupported(f"function returns '{qn}' by value which has a deleted copy constructor")
            # By-value returns are heap-allocated and handed to the caller,
            # who owns the result and must call the matching _destroy().
            return ReturnSpec(
                c_type=f"{info.c_type}*",
                stmt_template=f"return reinterpret_cast<{info.c_type}*>(new {qn}({{call}}));",
                needs_free_doc=True,
            )

        raise Unsupported(f"unsupported return type '{spelling}'")

    def _shared_ptr_return_spec(self, info, qn) -> ReturnSpec:
        # Always allocate a fresh shared_ptr<X> copy (bumping the
        # refcount), even for a `const X&`/pointer return: the alternative
        # -- handing out a handle that aliases the callee's own member
        # shared_ptr -- would dangle the moment that member is reassigned,
        # and C has no way to express "borrowed, and only until the next
        # call" the way a C++ reference's scope does. The caller owns the
        # result and must call the matching _release() (which drops this
        # reference, not necessarily the underlying object).
        return ReturnSpec(
            c_type=f"{info.c_type}*",
            stmt_template=f"return reinterpret_cast<{info.c_type}*>(new std::shared_ptr<{qn}>({{call}}));",
            needs_free_doc=True,
        )

    def _bind_indirect_return(self, pointee, orig_spelling, was_pointer):
        is_const = pointee.is_const_qualified()
        # A pointer-typed call expression already yields an address; a
        # reference-typed one is an lvalue and needs `&` to get its address.
        addr_of = "" if was_pointer else "&"

        if pointee.kind == TypeKind.VOID:
            c_type = "const void*" if is_const else "void*"
            return ReturnSpec(c_type=c_type, stmt_template="return {call};")

        if pointee.kind == TypeKind.CHAR_S and is_const and was_pointer:
            return ReturnSpec(c_type="const char*", stmt_template="return {call};")

        sp_hit = self._shared_ptr_record(pointee)
        if sp_hit is not None:
            info, qn = sp_hit
            # Copy-constructing a new shared_ptr<X> from `{call}` works the
            # same whether the original return was by value, `const X&`,
            # or (rare) `X*` -- no addr_of/deref juggling needed here.
            return self._shared_ptr_return_spec(info, qn)

        if self._is_primitivelike(pointee):
            base = self._primitive_c_type(pointee)
            c_type = f"const {base}*" if is_const else f"{base}*"
            return ReturnSpec(c_type=c_type, stmt_template="return {call};" if was_pointer
                              else f"return {addr_of}({{call}});")

        enum_hit = self._enum_info(pointee)
        if enum_hit is not None:
            info, qn = enum_hit
            if info is None:
                raise Unsupported(f"function returns pointer/reference to enum '{qn}' outside the wrapped API")
            c_type = f"const {info.c_type}*" if is_const else f"{info.c_type}*"
            return ReturnSpec(
                c_type=c_type,
                stmt_template=f"return reinterpret_cast<{c_type}>({addr_of}({{call}}));",
            )

        record_hit = self._record_info(pointee)
        if record_hit is not None:
            info, qn = record_hit
            if info is None:
                raise Unsupported(f"function returns pointer/reference to '{qn}' outside the wrapped API")
            c_type = f"const {info.c_type}*" if is_const else f"{info.c_type}*"
            # This is a *borrowed* pointer/reference into engine-owned
            # memory: no ownership transfer, caller must not destroy it.
            return ReturnSpec(
                c_type=c_type,
                stmt_template=f"return reinterpret_cast<{c_type}>({addr_of}({{call}}));",
            )

        raise Unsupported(f"unsupported pointer/reference return pointee '{pointee.spelling}'")
