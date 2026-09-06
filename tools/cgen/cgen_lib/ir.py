"""Intermediate representation produced by the scanner and consumed by the
emitter. Kept deliberately dumb (plain dataclasses) so the two stages don't
need to know about libclang or about C source text respectively.
"""
from dataclasses import dataclass, field
from typing import List, Optional


@dataclass
class Param:
    name: str
    cpp_type_spelling: str
    c_type: str
    cpp_arg: str  # C++ expression (using `name`) to pass to the real call


@dataclass
class ReturnSpec:
    c_type: str
    # Template with a single {call} placeholder standing in for the C++
    # expression that invokes the wrapped function/method, e.g.
    # "return {call};" or "return smlt_c_strdup({call}.c_str());"
    stmt_template: str
    needs_free_doc: bool = False  # caller must free()/smlt_x_destroy()/smlt_x_release() the result


@dataclass
class Method:
    cpp_name: str
    c_name: str
    params: List[Param]
    return_spec: ReturnSpec
    call_expr: str = ""
    is_static: bool = False
    is_const: bool = False
    is_constructor: bool = False
    is_destructor: bool = False
    is_operator: bool = False
    brief: str = ""


@dataclass
class ClassBinding:
    cpp_qualified_name: str
    base_name: str  # e.g. "ui_widget"
    c_type: str  # e.g. "smlt_ui_widget_t"
    methods: List[Method] = field(default_factory=list)
    is_abstract: bool = False
    source_file: str = ""
    extra_includes: List[str] = field(default_factory=list)

    @property
    def has_create(self):
        return any(m.is_constructor for m in self.methods)

    @property
    def has_destroy(self):
        return any(m.is_destructor for m in self.methods)


@dataclass
class Enumerator:
    c_name: str
    cpp_name: str
    value: int


@dataclass
class EnumBinding:
    cpp_qualified_name: str
    base_name: str
    c_type: str
    enumerators: List[Enumerator]
    source_file: str = ""


@dataclass
class FreeFunction:
    cpp_qualified_name: str
    c_name: str
    params: List[Param]
    return_spec: ReturnSpec
    call_expr: str = ""
    source_file: str = ""
    extra_includes: List[str] = field(default_factory=list)


@dataclass
class OpaqueForwardDecl:
    """A class we know exists (e.g. it's used as a Scene* parameter) but that
    we are not generating full bindings for, either because it wasn't in the
    scan input or because it turned out to be unwrappable. We still emit an
    opaque typedef for it so pointers to it can flow through the API.
    """
    cpp_qualified_name: str
    base_name: str
    c_type: str


@dataclass
class TranslationUnit:
    classes: List[ClassBinding] = field(default_factory=list)
    enums: List[EnumBinding] = field(default_factory=list)
    functions: List[FreeFunction] = field(default_factory=list)
    opaque_types: List[OpaqueForwardDecl] = field(default_factory=list)
    opaque_enum_types: List[OpaqueForwardDecl] = field(default_factory=list)
