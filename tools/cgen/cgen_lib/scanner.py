"""Two-pass libclang-based scanner.

Pass 1 (``_collect``) walks the whole translation unit and registers every
non-template record/enum declared inside namespace ``smlt`` so that type
resolution in pass 2 works regardless of header include order.

Pass 2 (``_extract``) walks the same tree again and builds the IR: for each
concrete class/struct it binds public constructors, the destructor, public
methods, a curated set of operators, and public fields; anything it can't
map to a consistent C signature is dropped with a logged warning, not a
hard failure.
"""
from clang.cindex import AccessSpecifier, CursorKind

from . import clangutil, log, naming
from .ir import (ClassBinding, Enumerator, EnumBinding, FreeFunction, Method,
                  OpaqueForwardDecl, Param, ReturnSpec, TranslationUnit)
from .registry import Registry
from .typemap import TypeMapper, Unsupported

BINARY_OP_MAP = {
    "==": "equals", "!=": "not_equals",
    "<": "less_than", "<=": "less_equal",
    ">": "greater_than", ">=": "greater_equal",
    "+": "add", "-": "subtract", "*": "multiply", "/": "divide",
}
UNARY_OP_MAP = {
    "-": "negated", "!": "logical_not", "~": "bitwise_not",
}


def iter_smlt_cursors(root_cursor):
    """Yields every cursor declared (directly or transitively) inside
    namespace `smlt`, at any depth, while never descending into templates
    or into anything outside `smlt` (std, __gnu_cxx, vendored deps that
    live in their own namespace, ...).
    """
    def recurse(cursor, inside):
        for child in cursor.get_children():
            k = child.kind
            if k == CursorKind.NAMESPACE:
                yield from recurse(child, inside or child.spelling == "smlt")
                continue
            if k == CursorKind.LINKAGE_SPEC:
                yield from recurse(child, inside)
                continue
            if not inside:
                continue
            yield child
            if k in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL) and child.get_num_template_arguments() < 0:
                yield from recurse(child, True)
            # CLASS_TEMPLATE / FUNCTION_TEMPLATE and explicit/partial
            # specializations (get_num_template_arguments() >= 0) are
            # deliberately not recursed into: a nested class declared
            # inside e.g. `class Future<void> { struct FutureState {...}; }`
            # can't be referenced from generated code without repeating the
            # `<void>` specialization arguments, which we don't reconstruct.

    yield from recurse(root_cursor, False)


class Scanner:
    def __init__(self, path_filter, ignore_set):
        self.path_filter = path_filter
        self.ignore_set = ignore_set
        self.registry = Registry()
        self.type_mapper = TypeMapper(self.registry)
        self._warned_templates = set()
        self._derived_from_cache = {}

        # Classes whose real construction/destruction is owned by some
        # manager rather than a plain constructor/destructor call, so a
        # mechanical `new`/`delete` `_create`/`_destroy` would be actively
        # misleading (the object would never be registered with whatever
        # actually drives it). See ports/c/README.md's "StageNode create()"
        # section for the concrete case this was written for.
        self.MANAGED_LIFETIME_BASES = ("smlt::StageNode", "smlt::Asset")

    def _in_project(self, cursor) -> bool:
        f = cursor.location.file
        if f is None:
            return False
        return self.path_filter(f.name)

    def _ignored(self, cursor) -> bool:
        return clangutil.qualified_name(cursor) in self.ignore_set

    def _is_accessible(self, cursor) -> bool:
        """False for a nested class/struct/enum declared private or
        protected inside another class -- e.g. ArgParser::DefinedArg.
        Top-level (namespace-scope) declarations always pass: their
        access_specifier is INVALID, not PUBLIC, since they aren't a
        class member at all.
        """
        parent = cursor.semantic_parent
        if parent is not None and parent.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
            return cursor.access_specifier == AccessSpecifier.PUBLIC
        return True

    # ---------------------------------------------------------------- pass 1
    def collect(self, tu_cursor):
        for cursor in iter_smlt_cursors(tu_cursor):
            k = cursor.kind
            if k in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
                if not self._in_project(cursor) or not self._is_accessible(cursor):
                    continue
                if cursor.get_num_template_arguments() >= 0:
                    # An explicit/partial template specialization: its real
                    # C++ spelling needs angle-bracket template arguments
                    # that we don't reconstruct, so it can't be referenced
                    # consistently from generated code.
                    qn = clangutil.qualified_name(cursor)
                    if qn not in self._warned_templates:
                        self._warned_templates.add(qn)
                        log.warning(f"'{qn}' is an explicit template specialization and cannot be "
                                    f"wrapped consistently in C; skipping")
                    continue
                qn = clangutil.qualified_name(cursor)
                ns_parts, chain = clangutil.namespace_parts_and_chain(cursor)
                info = self.registry.register_record(qn, ns_parts, chain + [cursor.spelling],
                                                      cursor.location.file.name, k == CursorKind.STRUCT_DECL)
                if cursor.is_definition():
                    # A forward declaration may well have been registered
                    # first (e.g. compat.h forward-declares half the math
                    # types); once we see the real definition, that's the
                    # file that needs #including for a full type.
                    info.source_file = cursor.location.file.name
                    info.uses_shared_ptr = (qn == "smlt::Asset" or clangutil.is_derived_from(
                        cursor, "smlt::Asset", self._derived_from_cache))
                    for member in cursor.get_children():
                        if member.kind == CursorKind.CONSTRUCTOR and member.is_copy_constructor() \
                                and member.is_deleted_method():
                            info.is_copyable = False
            elif k == CursorKind.ENUM_DECL:
                if not self._in_project(cursor) or not self._is_accessible(cursor):
                    continue
                qn = clangutil.qualified_name(cursor)
                ns_parts, chain = clangutil.namespace_parts_and_chain(cursor)
                self.registry.register_enum(qn, ns_parts, chain + [cursor.spelling],
                                             cursor.location.file.name)

    # ---------------------------------------------------------------- pass 2
    def extract(self, tu_cursor) -> TranslationUnit:
        out = TranslationUnit()
        seen_functions = {}  # proposed_c_name -> list[(FreeFunction, cursor)]

        for cursor in iter_smlt_cursors(tu_cursor):
            k = cursor.kind
            qn = clangutil.qualified_name(cursor)

            if k == CursorKind.CLASS_TEMPLATE:
                if qn not in self._warned_templates:
                    self._warned_templates.add(qn)
                    log.warning(f"'{qn}' is a class template and cannot be wrapped "
                                f"consistently in C; skipping")
                continue

            if k == CursorKind.FUNCTION_TEMPLATE and cursor.semantic_parent.kind != CursorKind.CLASS_DECL \
                    and cursor.semantic_parent.kind != CursorKind.STRUCT_DECL:
                if qn not in self._warned_templates:
                    self._warned_templates.add(qn)
                    log.warning(f"'{qn}' is a function template and cannot be wrapped "
                                f"consistently in C; skipping")
                continue

            if k in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
                if not cursor.is_definition() or not self._in_project(cursor):
                    continue
                if cursor.get_num_template_arguments() >= 0 or qn not in self.registry.records:
                    continue  # explicit specialization, already warned about in collect()
                if self._ignored(cursor):
                    log.info(f"'{qn}' is in the ignore list, skipping")
                    continue
                binding = self._extract_class(cursor)
                if binding is not None:
                    out.classes.append(binding)
                    self.registry.records[qn].is_wrapped = True

            elif k == CursorKind.ENUM_DECL:
                if not self._in_project(cursor) or not self._is_accessible(cursor):
                    continue
                if self._ignored(cursor):
                    continue
                binding = self._extract_enum(cursor)
                if binding is not None:
                    out.enums.append(binding)
                    self.registry.enums[qn].is_wrapped = True

            elif k == CursorKind.FUNCTION_DECL:
                if not self._in_project(cursor) or self._ignored(cursor):
                    continue
                self._extract_free_function(cursor, seen_functions)

        for candidates in seen_functions.values():
            self._flush_overloads(candidates, out.functions)

        # Anything referenced as a pointer/reference but never fully
        # wrapped (forward-declared only, abstract with no usable ctor
        # context, or simply not part of this scan) still needs an opaque
        # typedef so pointers to it can be passed around.
        for info in self.registry.records.values():
            if not info.is_wrapped:
                out.opaque_types.append(OpaqueForwardDecl(info.qualified_name, info.base_name, info.c_type))
        for info in self.registry.enums.values():
            if not info.is_wrapped:
                out.opaque_enum_types.append(OpaqueForwardDecl(info.qualified_name, info.base_name, info.c_type))

        return out

    # -- classes -----------------------------------------------------------
    def _extract_class(self, cursor):
        qn = clangutil.qualified_name(cursor)
        ns_parts, chain = clangutil.namespace_parts_and_chain(cursor)
        base_name = naming.c_type_base_name(ns_parts, chain + [cursor.spelling])
        c_type = naming.c_type_name(base_name)
        own_source_file = cursor.location.file.name
        self.type_mapper.reset_tracking()
        # is_abstract_record() accounts for pure virtuals inherited from a
        # base class and never overridden, not just ones declared directly
        # on this class (e.g. Light doesn't declare any pure virtuals of
        # its own, but inherits StageNode::node_params() unimplemented).
        is_abstract = cursor.is_abstract_record()

        # Some classes are only ever legitimately constructed/destroyed
        # through an owning manager (StageNode subclasses go through
        # StageNodeManager's slab allocator and per-frame update sweep --
        # see ports/c/stage_node_ext.h). A plain `new`/`delete` would
        # compile but produce an object the engine never drives or tracks,
        # so we don't generate create()/destroy() for these at all; other
        # instance methods are still wrapped normally since those work
        # correctly on a pointer obtained however it legitimately was.
        has_managed_lifetime = qn in self.MANAGED_LIFETIME_BASES or any(
            clangutil.is_derived_from(cursor, base, self._derived_from_cache)
            for base in self.MANAGED_LIFETIME_BASES)

        # Asset subclasses are additionally reference-counted: every real
        # instance lives inside a std::shared_ptr<T> owned by AssetManager
        # (and possibly other shared_ptrs elsewhere), never a bare T*. So a
        # `smlt_x_t*` handle for one of these classes always actually
        # points at a heap-allocated std::shared_ptr<T> wrapper, not a T
        # directly -- every method/field access below needs an extra
        # dereference through that wrapper, and destroy drops the
        # reference (deletes the shared_ptr) rather than the object.
        uses_shared_ptr = qn == "smlt::Asset" or clangutil.is_derived_from(
            cursor, "smlt::Asset", self._derived_from_cache)

        # A class only gets an explicit DESTRUCTOR cursor in the AST if it
        # declares one itself; the common case (implicit, compiler-provided
        # destructor) has none at all, and that still means "destructible".
        # We only need to special-case an *explicit* private/protected or
        # `= delete`d destructor.
        is_destructible = True
        for member in cursor.get_children():
            if member.kind == CursorKind.DESTRUCTOR:
                is_destructible = (member.access_specifier == AccessSpecifier.PUBLIC
                                   and not member.is_deleted_method())
                break

        binding = ClassBinding(cpp_qualified_name=qn, base_name=base_name, c_type=c_type,
                                is_abstract=is_abstract, source_file=cursor.location.file.name)

        if uses_shared_ptr:
            # Named _release rather than _destroy: this deletes the
            # std::shared_ptr<T> wrapper, dropping this one reference --
            # it does not necessarily destroy T (something else, e.g.
            # AssetManager's own cache, may still hold a reference), and
            # "destroy" already means something specific and different
            # elsewhere in the engine (e.g. StageNode::destroy()). T's own
            # destructor accessibility is irrelevant here, since we never
            # call it directly.
            binding.methods.append(Method(
                cpp_name="~" + cursor.spelling,
                c_name=f"{naming.SMLT_PREFIX}_{base_name}_release",
                params=[Param("self", qn, f"{c_type}*", "")],
                return_spec=ReturnSpec(
                    c_type="void",
                    stmt_template=f"delete reinterpret_cast<std::shared_ptr<{qn}>*>({{call}});"),
                call_expr="self",
                is_destructor=True,
            ))
        elif has_managed_lifetime:
            log.info(f"'{qn}' has a manager-owned lifetime (derives from StageNode): "
                     f"no {naming.SMLT_PREFIX}_{base_name}_destroy() or _create() will be generated")
        elif is_destructible:
            binding.methods.append(Method(
                cpp_name="~" + cursor.spelling,
                c_name=f"{naming.SMLT_PREFIX}_{base_name}_destroy",
                params=[Param("self", qn, f"{c_type}*", "")],
                return_spec=ReturnSpec(c_type="void",
                                      stmt_template=f"delete reinterpret_cast<{qn}*>({{call}});"),
                call_expr="self",
                is_destructor=True,
            ))
        else:
            log.info(f"'{qn}' has an inaccessible or deleted destructor: no "
                     f"{naming.SMLT_PREFIX}_{base_name}_destroy() or _create() will be generated")

        ctor_candidates = []
        method_candidates = {}  # proposed_c_name -> list[(Method, cursor)]

        # Unlike the destructor, whether an implicit default constructor
        # exists depends on base classes and member initializers in ways
        # libclang doesn't expose a direct query for, so (unlike _destroy)
        # we don't speculatively synthesize _create() when no constructor
        # is declared -- only explicit, user-declared constructors below
        # ever produce a _create().

        for member in cursor.get_children():
            if member.access_specifier != AccessSpecifier.PUBLIC:
                continue
            if self._ignored(member):
                log.info(f"'{clangutil.qualified_name(member)}' is in the ignore list, skipping")
                continue

            mk = member.kind
            if mk == CursorKind.CONSTRUCTOR:
                if is_abstract or not is_destructible or has_managed_lifetime:
                    continue
                if member.is_copy_constructor() or member.is_move_constructor():
                    continue
                if member.is_deleted_method():
                    continue
                self._try_bind_constructor(member, qn, base_name, c_type, ctor_candidates)

            elif mk == CursorKind.CXX_METHOD:
                self._try_bind_method(member, qn, base_name, c_type, method_candidates, uses_shared_ptr)

            elif mk == CursorKind.FUNCTION_TEMPLATE:
                fqn = clangutil.qualified_name(member)
                if fqn not in self._warned_templates:
                    self._warned_templates.add(fqn)
                    log.warning(f"'{fqn}' is a template method and cannot be wrapped "
                                f"consistently in C; skipping")

            elif mk == CursorKind.FIELD_DECL:
                self._try_bind_field(member, qn, base_name, c_type, binding, uses_shared_ptr)

            # nested CLASS_DECL/STRUCT_DECL/ENUM_DECL are handled by the
            # outer traversal in extract(), not here.

        self._flush_overloads(ctor_candidates, binding.methods)
        for candidates in method_candidates.values():
            self._flush_overloads(candidates, binding.methods)

        self._resolve_name_collisions(qn, binding.methods)

        if not binding.methods:
            log.info(f"'{qn}' produced no bindable members")

        binding.extra_includes = sorted(self.type_mapper.touched_source_files - {own_source_file})
        return binding

    def _resolve_name_collisions(self, qn, methods):
        """The synthesized destructor/field accessor names can collide with
        an ordinary method that happens to share the name (e.g. a real
        `destroy()` method alongside the synthesized `_destroy` wrapper for
        the C++ destructor). Constructors, the destructor, and fields are
        added to `methods` before regular overloaded methods are flushed
        into it, so on a collision the first (structural) entry keeps the
        canonical name and later entries get a numeric suffix.
        """
        seen = {}
        for m in methods:
            if m.c_name not in seen:
                seen[m.c_name] = 1
                continue
            original = m.c_name
            while True:
                seen[original] += 1
                candidate = f"{original}_{seen[original]}"
                if candidate not in seen:
                    break
            m.c_name = candidate
            seen[candidate] = 1
            log.warning(f"'{qn}': generated name '{original}' collides with another member, "
                        f"renamed this one to '{candidate}'")

    def _try_bind_constructor(self, member, qn, base_name, c_type, ctor_candidates):
        ctx = f"{qn}::{member.spelling} (constructor)"
        try:
            params = self._bind_params(member, ctx)
        except Unsupported as e:
            log.warning(f"{ctx}: {e.reason}, skipping this overload")
            return
        proposed = f"{naming.SMLT_PREFIX}_{base_name}_create"
        args = ", ".join(p.cpp_arg for p in params)
        method = Method(
            cpp_name=member.spelling, c_name=proposed, params=params,
            return_spec=ReturnSpec(c_type=f"{c_type}*",
                                  stmt_template=f"return reinterpret_cast<{c_type}*>(new {{call}});"),
            call_expr=f"{qn}({args})",
            is_constructor=True,
        )
        ctor_candidates.append((proposed, method, member))

    def _self_receiver(self, qn, is_const, uses_shared_ptr):
        """C++ expression that turns the `self` C parameter into something
        `->member` can be chained off of. Asset subclasses store a
        std::shared_ptr<T>* behind the handle instead of a T* (see
        `uses_shared_ptr` in _extract_class), so reaching the real object
        needs an extra dereference; shared_ptr's own operator-> then
        reaches the underlying T from there.
        """
        if uses_shared_ptr:
            sp_type = f"const std::shared_ptr<{qn}>*" if is_const else f"std::shared_ptr<{qn}>*"
            return f"(*reinterpret_cast<{sp_type}>(self))"
        receiver_cast = f"const {qn}*" if is_const else f"{qn}*"
        return f"reinterpret_cast<{receiver_cast}>(self)"

    def _try_bind_method(self, member, qn, base_name, c_type, method_candidates, uses_shared_ptr):
        name = member.spelling
        ctx = f"{qn}::{name}"

        if name.startswith("operator"):
            symbol = name[len("operator"):].strip()
            nparams = len(list(member.get_arguments()))
            if nparams == 1 and symbol in BINARY_OP_MAP:
                mapped = BINARY_OP_MAP[symbol]
            elif nparams == 0 and symbol in UNARY_OP_MAP:
                mapped = UNARY_OP_MAP[symbol]
            else:
                log.warning(f"{ctx}: operator '{name}' has no consistent C equivalent, skipping")
                return
            method_name_for_c = mapped
            is_operator = True
        else:
            method_name_for_c = name
            is_operator = False

        try:
            params = self._bind_params(member, ctx)
            return_spec = self.type_mapper.bind_return(member.result_type, ctx)
        except Unsupported as e:
            log.warning(f"{ctx}: {e.reason}, skipping")
            return

        is_static = member.is_static_method()
        is_const = member.is_const_method()
        args = ", ".join(p.cpp_arg for p in params)

        if is_static:
            call_expr = f"{qn}::{name}({args})"
        else:
            self_type = f"const {c_type}*" if is_const else f"{c_type}*"
            params = [Param("self", qn, self_type, "")] + params
            receiver = self._self_receiver(qn, is_const, uses_shared_ptr)
            call_expr = f"{receiver}->{name}({args})"

        proposed = naming.c_func_name(base_name, method_name_for_c)
        method = Method(cpp_name=name, c_name=proposed, params=params, return_spec=return_spec,
                        call_expr=call_expr, is_static=is_static, is_const=is_const,
                        is_operator=is_operator)
        method_candidates.setdefault(proposed, []).append((proposed, method, member))

    def _try_bind_field(self, member, qn, base_name, c_type, binding, uses_shared_ptr):
        field_name = member.spelling
        ctx = f"{qn}::{field_name} (field)"
        getter_name = naming.c_func_name(base_name, f"get_{field_name}")
        setter_name = naming.c_func_name(base_name, f"set_{field_name}")
        try:
            ret = self.type_mapper.bind_return(member.type, ctx)
        except Unsupported as e:
            log.warning(f"{ctx}: {e.reason}, skipping field accessors")
            return
        getter_receiver = self._self_receiver(qn, True, uses_shared_ptr)
        binding.methods.append(Method(
            cpp_name=field_name, c_name=getter_name,
            params=[Param("self", qn, f"const {c_type}*", "")],
            return_spec=ret, is_const=True,
            call_expr=f"{getter_receiver}->{field_name}",
        ))
        if member.type.is_const_qualified():
            return
        try:
            setter_param = self.type_mapper.bind_param(member.type, "value", ctx)
        except Unsupported as e:
            log.info(f"{ctx}: {e.reason}, skipping setter (getter still generated)")
            return
        setter_receiver = self._self_receiver(qn, False, uses_shared_ptr)
        binding.methods.append(Method(
            cpp_name=field_name, c_name=setter_name,
            params=[Param("self", qn, f"{c_type}*", ""), setter_param],
            return_spec=ReturnSpec(c_type="void", stmt_template="{call};"),
            call_expr=f"{setter_receiver}->{field_name} = {setter_param.cpp_arg}",
        ))

    def _bind_params(self, member, ctx):
        params = []
        for i, arg in enumerate(member.get_arguments()):
            name = arg.spelling or f"arg{i}"
            params.append(self.type_mapper.bind_param(arg.type, name, ctx))
        if member.type.is_function_variadic():
            raise Unsupported("variadic functions are not supported")
        return params

    # -- enums ---------------------------------------------------------------
    def _extract_enum(self, cursor):
        qn = clangutil.qualified_name(cursor)
        ns_parts, chain = clangutil.namespace_parts_and_chain(cursor)
        base_name = naming.c_type_base_name(ns_parts, chain + [cursor.spelling])
        c_type = naming.c_type_name(base_name)
        enumerators = []
        for child in cursor.get_children():
            if child.kind != CursorKind.ENUM_CONSTANT_DECL:
                continue
            c_name = naming.c_enum_value_name(cursor.spelling, child.spelling)
            enumerators.append(Enumerator(c_name=c_name, cpp_name=child.spelling,
                                           value=child.enum_value))
        if not enumerators:
            log.info(f"enum '{qn}' has no enumerators, skipping")
            return None
        return EnumBinding(cpp_qualified_name=qn, base_name=base_name, c_type=c_type,
                           enumerators=enumerators, source_file=cursor.location.file.name)

    # -- free functions --------------------------------------------------------
    def _extract_free_function(self, cursor, seen_functions):
        qn = clangutil.qualified_name(cursor)
        name = cursor.spelling
        ns_parts, _ = clangutil.namespace_parts_and_chain(cursor)
        ctx = qn

        if name.startswith("operator"):
            log.info(f"{ctx}: free operator overloads are not wrapped, skipping")
            return

        if cursor.get_num_template_arguments() >= 0:
            # An explicit specialization of a function template: calling it
            # unqualified needs angle-bracket template arguments we don't
            # reconstruct (same issue as class template specializations).
            if qn not in self._warned_templates:
                self._warned_templates.add(qn)
                log.warning(f"'{qn}' is an explicit function template specialization and cannot "
                            f"be wrapped consistently in C; skipping")
            return

        own_source_file = cursor.location.file.name
        self.type_mapper.reset_tracking()
        try:
            params = self._bind_params(cursor, ctx)
            return_spec = self.type_mapper.bind_return(cursor.result_type, ctx)
        except Unsupported as e:
            log.warning(f"{ctx}: {e.reason}, skipping")
            return

        proposed = naming.c_free_func_name(ns_parts, name)
        args = ", ".join(p.cpp_arg for p in params)
        extra_includes = sorted(self.type_mapper.touched_source_files - {own_source_file})
        func = FreeFunction(cpp_qualified_name=qn, c_name=proposed, params=params,
                            return_spec=return_spec, call_expr=f"{qn}({args})",
                            source_file=own_source_file, extra_includes=extra_includes)
        seen_functions.setdefault(proposed, []).append((proposed, func, cursor))

    # -- shared: overload disambiguation ---------------------------------------
    def _flush_overloads(self, candidates, out_list):
        if not candidates:
            return
        # Deterministic order: by parameter count, then by the joined
        # parameter C types, so re-running the generator is stable.
        candidates = sorted(candidates, key=lambda c: (len(c[1].params),
                                                       ",".join(p.c_type for p in c[1].params)))
        names = [c[0] for c in candidates]
        unique_names = naming.disambiguate(names)
        for (_, obj, _cursor), new_name in zip(candidates, unique_names):
            obj.c_name = new_name
            out_list.append(obj)
