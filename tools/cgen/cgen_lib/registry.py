"""Tracks every non-template record (class/struct) and enum declared inside
the `smlt` namespace that the scanner has seen, keyed by fully-qualified C++
name. Populated in a first pass over the AST so that type mapping in the
second pass can resolve forward references regardless of header include
order (e.g. Stage's constructor takes a `Scene*` even if scene.h happens to
be scanned after stage.h).
"""
from dataclasses import dataclass
from typing import Dict, List, Optional

from . import naming


@dataclass
class RecordInfo:
    qualified_name: str
    base_name: str
    c_type: str
    is_struct: bool
    source_file: str
    is_wrapped: bool = False  # True once full bindings were generated for it
    is_copyable: bool = True  # False if an explicit copy ctor/assign is deleted
    uses_shared_ptr: bool = False  # True for smlt::Asset subclasses -- see typemap.py


@dataclass
class EnumInfo:
    qualified_name: str
    base_name: str
    c_type: str
    source_file: str
    is_wrapped: bool = False


class Registry:
    def __init__(self):
        self.records: Dict[str, RecordInfo] = {}
        self.enums: Dict[str, EnumInfo] = {}

    def register_record(self, qualified_name: str, namespace_parts: List[str],
                         name_chain: List[str], source_file: str, is_struct: bool) -> RecordInfo:
        if qualified_name in self.records:
            return self.records[qualified_name]
        base_name = naming.c_type_base_name(namespace_parts, name_chain)
        info = RecordInfo(
            qualified_name=qualified_name,
            base_name=base_name,
            c_type=naming.c_type_name(base_name),
            is_struct=is_struct,
            source_file=source_file,
        )
        self.records[qualified_name] = info
        return info

    def register_enum(self, qualified_name: str, namespace_parts: List[str],
                       name_chain: List[str], source_file: str) -> EnumInfo:
        if qualified_name in self.enums:
            return self.enums[qualified_name]
        base_name = naming.c_type_base_name(namespace_parts, name_chain)
        info = EnumInfo(
            qualified_name=qualified_name,
            base_name=base_name,
            c_type=naming.c_type_name(base_name),
            source_file=source_file,
        )
        self.enums[qualified_name] = info
        return info

    def lookup_record(self, qualified_name: str) -> Optional[RecordInfo]:
        return self.records.get(qualified_name)

    def lookup_enum(self, qualified_name: str) -> Optional[EnumInfo]:
        return self.enums.get(qualified_name)
