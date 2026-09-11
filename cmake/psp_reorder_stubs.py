#!/usr/bin/env python3
"""Regroups a PSP ELF's import stubs so psp-fixup-imports can do its job.

Run on the linked ELF immediately *before* psp-fixup-imports (see
toolchains/CreatePBP.cmake). Without this, larger programs get imports that
silently resolve to the wrong syscall, or to nothing at all.

Background
----------
After linking, every imported PSP function has an 8-byte entry in
.sceStub.text and a matching 4-byte NID in .rodata.sceNid (parallel arrays),
and every imported *module* has one PspModuleImport descriptor in .lib.stub.
psp-fixup-imports fills in each descriptor's nids/funcs pointers and
func_count, and the loader then reads func_count *consecutive* entries
starting at nids/funcs -- so all of one module's entries have to be
physically contiguous in those two arrays.

Upstream only sets nids/funcs on a module's first entry and increments
func_count for each later one wherever it lands, so when a module's entries
aren't contiguous it prints "could not fixup imports, stubs out of order"
and then writes a descriptor claiming a run it doesn't actually have. The
claimed range spills into whatever unrelated modules' entries physically
follow, and those imports resolve to the wrong thing.

Whether the entries *are* contiguous depends on the order the linker happens
to pull per-function stub objects out of libpspuser.a/libpspkernel.a. For a
program linking many static libraries (engine + C bindings + Vala bindings +
glib + gobject + pcre2 + libffi) they're routinely interleaved, and link
order can't fix it -- "link the SDK libraries last" is already done here.
Two consequences seen on this project, both confirmed in PPSSPP:

  - sceDisplaySetMode/sceDisplaySetFrameBuf never resolving, so nothing was
    ever displayed (a permanently black screen).
  - sceKernelWaitSema/sceKernelSignalSema resolving at some call sites and
    silently becoming no-ops at others, quietly breaking every Mutex in
    simulant/threads/mutex.cpp.

What this does
--------------
Stably regroups the two arrays by each entry's owning descriptor -- groups
ordered by first appearance, entries keeping their original order within a
group -- so the unmodified upstream tool then sees exactly the contiguous
runs it assumes. Sections keep their original size and position; only the
contents of the two parallel arrays are permuted.

Everything that referred to an entry by its old position is then fixed up:

  1. .rel.sceStub.text relocation offsets, so each entry's R_MIPS_32
     relocation follows it.
  2. jal call sites -- the part that actually matters at runtime. This is a
     fully linked ELF using REL (not RELA) relocations, so R_MIPS_26 carries
     no separate addend: the resolved target is encoded in the instruction's
     low 26 bits, and the retained relocation exists only so the PRX loader
     can re-base it. psp-prxgen then drops the symbol table entirely, so the
     loader never re-resolves from symbols -- it just adds the load-base
     delta to what's already encoded. Every jal therefore has to be
     rewritten, or callers keep jumping into whatever moved into the old
     slot. (R_MIPS_26 in .rel.text is the only relocation type that
     references .sceStub.text -- nothing takes a stub's address, so there
     are no HI16/LO16 pairs to fix.)
  3. Symbol table entries pointing into .sceStub.text. Nothing at runtime
     reads these, but keeping them accurate means the ELF still
     disassembles correctly and psp-nm doesn't report stale addresses.

Re-running on an already-fixed-up ELF is a no-op: every entry then reads as
JR_31/NOP, which is treated as a singleton and never moved.
"""
import argparse
import struct
import sys

SHT_NOBITS = 8
SHT_REL = 9
R_MIPS_26 = 4
STT_SECTION = 3

MIPS_JR_31 = 0x03E00008
MIPS_NOP = 0x00000000

STUBTEXT_SECT = ".sceStub.text"
NID_SECT = ".rodata.sceNid"
LIBSTUB_SECT = ".lib.stub"

ENTRY_SIZE = 8  # bytes per .sceStub.text entry (two words)
NID_SIZE = 4  # bytes per .rodata.sceNid entry


class Section:
    __slots__ = ("index", "name", "type", "addr", "offset", "size", "link",
                 "info", "entsize")


def _u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def _u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def _put_u32(data, off, value):
    struct.pack_into("<I", data, off, value & 0xFFFFFFFF)


def parse_sections(data):
    if data[:4] != b"\x7fELF":
        raise RuntimeError("not an ELF file")
    if data[5] != 1:
        raise RuntimeError("only little-endian ELF is supported")

    shoff = _u32(data, 32)
    shentsize = _u16(data, 46)
    shnum = _u16(data, 48)
    shstrndx = _u16(data, 50)

    raw = []
    for i in range(shnum):
        base = shoff + i * shentsize
        s = Section()
        s.index = i
        s.name = None
        s.type = _u32(data, base + 4)
        s.addr = _u32(data, base + 12)
        s.offset = _u32(data, base + 16)
        s.size = _u32(data, base + 20)
        s.link = _u32(data, base + 24)
        s.info = _u32(data, base + 28)
        s.entsize = _u32(data, base + 36)
        raw.append((s, _u32(data, base)))

    strtab_off = raw[shstrndx][0].offset
    sections = []
    for s, name_off in raw:
        end = data.index(b"\0", strtab_off + name_off)
        s.name = data[strtab_off + name_off:end].decode("ascii", "replace")
        sections.append(s)
    return sections


def find_section(sections, name):
    for s in sections:
        if s.name == name:
            return s
    return None


def va_to_offset(sections, va):
    """Maps a virtual address to a file offset, like the tool's find_data()."""
    for s in sections:
        if s.type == SHT_NOBITS or s.size == 0:
            continue
        if s.addr <= va < s.addr + s.size:
            return s.offset + (va - s.addr)
    return None


def build_permutation(data, stubtext, count):
    """Stable group-by-owning-descriptor.

    Returns (new_order, new_index_of_old), where new_order[new] = old.
    Entries already in "original NID format" (JR_31/NOP, which upstream
    special-cases) get a unique key so they're never merged with anything.
    """
    keys = []
    for i in range(count):
        base = stubtext.offset + i * ENTRY_SIZE
        stub_addr = _u32(data, base)
        stub_nid = _u32(data, base + 4)
        if stub_addr != MIPS_JR_31 or stub_nid != MIPS_NOP:
            keys.append(("stub", stub_addr))
        else:
            keys.append(("singleton", i))

    rank_of = []
    seen = {}
    for key in keys:
        if key not in seen:
            seen[key] = len(seen)
        rank_of.append(seen[key])

    buckets = [[] for _ in range(len(seen))]
    for i, rank in enumerate(rank_of):
        buckets[rank].append(i)

    new_order = [i for bucket in buckets for i in bucket]
    new_index_of_old = [0] * count
    for new_index, old_index in enumerate(new_order):
        new_index_of_old[old_index] = new_index
    return new_order, new_index_of_old, len(seen)


def reorder(path, verbose=False):
    with open(path, "rb") as f:
        data = bytearray(f.read())

    sections = parse_sections(data)
    stubtext = find_section(sections, STUBTEXT_SECT)
    nids = find_section(sections, NID_SECT)
    libstub = find_section(sections, LIBSTUB_SECT)

    if stubtext is None or nids is None or libstub is None:
        # Not a PSP module with imports (or already stripped); nothing to do.
        if verbose:
            print("psp_reorder_stubs: no import sections, nothing to do")
        return False

    count = nids.size // NID_SIZE
    if count == 0:
        return False
    if stubtext.size != nids.size * 2:
        raise RuntimeError(
            "{} and {} sizes disagree ({} vs {})".format(
                STUBTEXT_SECT, NID_SECT, stubtext.size, nids.size))

    new_order, new_index_of_old, group_count = build_permutation(
        data, stubtext, count)

    if all(new_index_of_old[i] == i for i in range(count)):
        if verbose:
            print("psp_reorder_stubs: already grouped, nothing to do")
        return False

    # 1. Permute the two parallel arrays.
    old_entries = [
        bytes(data[stubtext.offset + i * ENTRY_SIZE:
                   stubtext.offset + (i + 1) * ENTRY_SIZE])
        for i in range(count)
    ]
    old_nids = [
        bytes(data[nids.offset + i * NID_SIZE:nids.offset + (i + 1) * NID_SIZE])
        for i in range(count)
    ]
    for new_index, old_index in enumerate(new_order):
        off = stubtext.offset + new_index * ENTRY_SIZE
        data[off:off + ENTRY_SIZE] = old_entries[old_index]
        off = nids.offset + new_index * NID_SIZE
        data[off:off + NID_SIZE] = old_nids[old_index]

    def remap_va(va):
        """Maps an address inside .sceStub.text to its post-move address."""
        rel = va - stubtext.addr
        old_index, within = divmod(rel, ENTRY_SIZE)
        return stubtext.addr + new_index_of_old[old_index] * ENTRY_SIZE + within

    def in_stubtext(va):
        return stubtext.addr <= va < stubtext.addr + stubtext.size

    # 2. .rel.sceStub.text -- move each relocation to follow its entry.
    moved_relocs = 0
    for s in sections:
        if s.type != SHT_REL or s.info != stubtext.index:
            continue
        for r in range(s.size // 8):
            off = s.offset + r * 8
            target = _u32(data, off)
            if in_stubtext(target):
                _put_u32(data, off, remap_va(target))
                moved_relocs += 1

    # 3. jal call sites -- the part that matters at runtime.
    moved_calls = 0
    for s in sections:
        if s.type != SHT_REL:
            continue
        for r in range(s.size // 8):
            off = s.offset + r * 8
            info = _u32(data, off + 4)
            if info & 0xFF != R_MIPS_26:
                continue
            instr_off = va_to_offset(sections, _u32(data, off))
            if instr_off is None:
                continue
            instr = _u32(data, instr_off)
            # J-type: target = (PC & 0xF0000000) | (imm << 2). These images
            # link low enough that the top nibble is always zero, so the
            # encoded field alone gives the address.
            target = (instr & 0x03FFFFFF) << 2
            if in_stubtext(target):
                new_target = remap_va(target)
                _put_u32(data, instr_off,
                         (instr & 0xFC000000) | ((new_target >> 2) & 0x03FFFFFF))
                moved_calls += 1

    # 4. Symbols pointing into .sceStub.text.
    moved_syms = 0
    symtab = find_section(sections, ".symtab")
    if symtab is not None:
        for i in range(symtab.size // 16):
            off = symtab.offset + i * 16
            value = _u32(data, off + 4)
            if not in_stubtext(value):
                continue
            # The .sceStub.text section symbol itself must stay at the
            # section start -- it doesn't describe one specific entry.
            if data[off + 12] & 0xF == STT_SECTION:
                continue
            _put_u32(data, off + 4, remap_va(value))
            moved_syms += 1

    with open(path, "wb") as f:
        f.write(data)

    if verbose:
        moved = sum(1 for i in range(count) if new_index_of_old[i] != i)
        print("psp_reorder_stubs: {} groups, {}/{} entries moved, "
              "{} call sites, {} relocs, {} symbols repointed".format(
                  group_count, moved, count, moved_calls, moved_relocs,
                  moved_syms))
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", help="linked PSP ELF to rewrite in place")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    try:
        reorder(args.elf, args.verbose)
    except Exception as e:  # noqa: BLE001 - surfaced as a build error
        print("psp_reorder_stubs: {}".format(e), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
