#!/usr/bin/env python3
"""Stages a patched copy of deps/glib for an embedded cross-compile.

Used for both supported console targets, which differ only in which patch
directory they're pointed at:

    stage_glib_for_target.py <glib_source> <staged_dest> <patches_dir>

with <patches_dir> being cmake/glib-patches/psp or
cmake/glib-patches/dreamcast. Every *.patch directly inside it is applied
to glib here; anything under a libffi/ subdirectory is instead wired into
libffi.wrap for Meson to apply later (only PSP has any -- see below).

Never touches the deps/glib submodule itself -- it's a plain clone of
upstream GNOME/glib (see .gitmodules), and any local commit made directly
in it wouldn't be fetchable by anyone else cloning this repo (unlike the
bounce/libgl/libAL/sh4zam submodules, which point at Simulant's own forks
specifically so they *can* carry local patches). Instead this copies the
whole tree (minus already-extracted Meson subprojects, which would
otherwise shadow the freshly wrap-fetched, patched ones this run needs)
to a scratch directory and applies our own small, dependency-free
unified-diff applier to it -- deliberately not shelling out to `patch`
(not installed on either the stock kazade/psp-sdk or kazade/dreamcast-sdk
Docker image) or relying on Meson's own `diff_files` wrap mechanism
for our *own* patches (that shells out to `git apply`, which -- for
reasons not fully root-caused, possibly related to nested nested nested
subprocess/HOME environment propagation through CMake's execute_process
-- was observed to intermittently fail with a "not a git repository"
error in this same container when invoked through CMake rather than a
bare shell, even though glib's own pcre2 patch uses the exact same
mechanism and (also observed, in the same environment) sometimes works
fine. Applying our own patches ourselves sidesteps that fragility
entirely; it doesn't touch pcre2's pre-existing wrap-based patch, which
is glib's own upstream concern, not introduced by this project).

See cmake/glib-patches/{psp,dreamcast}/ for what each patch does and why:

  - glib-psp-support.patch / glib-dreamcast-support.patch: the same four
    edits on both platforms, differing only in the platform name. Two
    host_system checks in gio/meson.build that glib already carves
    Windows/Android exceptions out of, for reasons that apply equally
    here (no BIND resolver, no arpa/nameser.h); glib/gnulib's and
    libcharset's meson.build, which both hardcode pic: true and so
    override the global -Db_staticpic=false these builds need (PSP's MIPS
    eabi target can't generate position-independent code at all, and KOS
    compiles everything -fno-PIC -fno-PIE); and glib/glib-unix.c, where
    one function has a hard #error on any platform it doesn't recognize
    -- given a branch matching the existing Hurd one, which reports "not
    supported" through glib's own error-reporting rather than failing to
    compile. Applied to the staged copy immediately, with our own
    unified-diff applier (see apply_unified_diff() below).
  - libffi/*.patch: adds 'psp' to a hardcoded list of host_system values
    libffi recognizes for MIPS (allow-psp-mips.patch), hand-expands a
    handful of l.d/s.d (double-precision FPU load/store) instructions in
    its O32 assembly trampoline into lwc1/swc1 pairs, since the Allegrex
    CPU's assembler profile rejects l.d/s.d outright -- there's no real
    double-precision FPU hardware on a PSP (fix-allegrex-fpu-ops.patch) --
    and strips the `.abicalls`/`.cpload`/`.cprestore` directives from that
    same O32 trampoline (disable-abicalls.patch): confirmed by actually
    booting a built sample in PPSSPP, that assembly file was the *only*
    object in the entire dependency chain (audited every .o in glib,
    gobject, pcre2, libintl, and libffi itself) compiled with GOT-relative
    PIC/CPIC calling -- baked into the .S source unconditionally, so
    -Db_staticpic=false (which fixes this for ordinary C code elsewhere in
    this build) has no effect on it -- which produced GOT16 relocations
    PPSSPP's loader can't resolve ("ARGH IT'S AN UNKNOWN RELOCATION"),
    since this is a statically-linked, non-relocatable-at-load PSP
    homebrew module, not the shared-library environment this trampoline
    assumes. Removing those directives makes the assembler emit ordinary
    absolute (%hi/%lo) addressing instead, identical to every other object
    in the build -- verified directly (no more GOT16/CALL16 relocations in
    the resulting .o, and PPSSPP no longer logs the relocation error).
    libffi's meson.build itself needs allow-psp-mips.patch already applied
    to even *configure* successfully (it errors out at that point if 'psp'
    isn't recognized), so unlike glib-psp-support.patch above, none of
    these three can be deferred until after `meson setup` -- they get
    wired into subprojects/libffi.wrap's own `diff_files` (the exact same
    Meson mechanism deps/glib's own subprojects/pcre2.wrap already uses to
    patch bundled pcre2 for Apple platforms), which Meson applies itself,
    via `git apply`, at wrap-extraction time, mid-`meson setup`.

    Dreamcast needs one, for a different reason
    (fix-sh-elf-symbol-prefix.patch). libffi's SH backend needs no
    host_system allow-list entry and its sysv.S assembles fine, but that
    file hardcodes `#define CNAME(x) x` -- above a comment reading "XXX
    these lose for some platforms, I'm sure" -- so every assembly entry
    point is emitted without the leading underscore that bare-metal sh-elf
    (and therefore KOS) puts on every C symbol. The C side then references
    _ffi_call_SYSV while the assembly defines ffi_call_SYSV, and the final
    link fails on libffi's own functions. The patch routes CNAME through
    __USER_LABEL_PREFIX__, which is '_' on sh-elf and empty on SH-Linux,
    so it's a no-op wherever the original was already correct.
"""
import argparse
import re
import sys
from pathlib import Path
import shutil


def _apply_hunk(lines: list, hunk_lines: list, old_start: int) -> None:
    """Applies one already-parsed @@ hunk's body (the +/-/context lines
    following the @@ header, not including it) to `lines` in place.
    `old_start` is the 1-based line number the hunk header claims for its
    first context/deletion line -- searched outward from there (rather
    than trusted exactly) in case the file has drifted slightly, same as
    a real `patch` would via fuzz.
    """
    old_block = [l[1:] for l in hunk_lines if l[0] in (" ", "-")]

    def matches_at(pos: int) -> bool:
        return lines[pos:pos + len(old_block)] == old_block

    search_order = [old_start - 1]
    for offset in range(1, 50):
        search_order += [old_start - 1 + offset, old_start - 1 - offset]
    pos = next((p for p in search_order if 0 <= p <= len(lines) - len(old_block) and matches_at(p)), None)
    if pos is None:
        raise RuntimeError(f"hunk context not found near line {old_start}: {old_block[:3]!r}...")

    new_block = [l[1:] for l in hunk_lines if l[0] in (" ", "+")]
    lines[pos:pos + len(old_block)] = new_block


def apply_unified_diff(patch_text: str, base_dir: Path) -> None:
    """A small, dependency-free unified-diff applier -- see module
    docstring for why this project doesn't shell out to `patch`/`git
    apply` for its own patches. Only needs to handle exactly the shape
    of patch this project itself generates (see cmake/glib-patches/psp/):
    plain unified diff, one or more files, `-p1`-style `a/`/`b/` prefixes.
    """
    file_diffs = re.split(r"(?m)^--- ", patch_text)[1:]
    for file_diff in file_diffs:
        file_diff = "--- " + file_diff
        lines = file_diff.splitlines()
        old_path = lines[0].split("\t")[0][len("--- a/"):]
        target = base_dir / old_path
        content_lines = target.read_text().splitlines()

        i = 2  # skip --- and +++ header lines
        while i < len(lines) and lines[i].startswith("@@"):
            m = re.match(r"@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@", lines[i])
            old_start = int(m.group(1))
            i += 1
            hunk_lines = []
            while i < len(lines) and not lines[i].startswith("@@") and not lines[i].startswith("--- "):
                hunk_lines.append(lines[i])
                i += 1
            _apply_hunk(content_lines, hunk_lines, old_start)

        target.write_text("\n".join(content_lines) + "\n")


def stage(glib_source: Path, staged: Path, patches_dir: Path) -> None:
    if staged.exists():
        shutil.rmtree(staged)

    def ignore_extracted_subprojects(dir_path, names):
        if Path(dir_path) == glib_source / "subprojects":
            # Keep .wrap files and packagefiles/, skip anything Meson has
            # already fetched/extracted here (e.g. libffi-3.5.2/,
            # pcre2-10.46/) -- those need to come from a fresh extraction
            # for *this* (PSP) build, not reuse a host build's.
            return [n for n in names if n != "packagefiles" and not n.endswith(".wrap")]
        return []

    shutil.copytree(glib_source, staged, ignore=ignore_extracted_subprojects)

    # deps/glib is a submodule -- its .git is a *file* (not a directory)
    # pointing at ../../.git/modules/deps/glib via a relative path that's
    # only valid from deps/glib's own real location. Copied verbatim into
    # the staged tree, it becomes a dangling reference at exactly the
    # spot any `git` command run from inside this tree would look first
    # when walking up to find "the" repository -- including Meson's own
    # `git apply` for pcre2's pre-existing wrap patch (see subprojects/
    # pcre2.wrap), which starts failing with a bare "fatal: not a git
    # repository" once that happens. The staged tree has no legitimate
    # need for git metadata of its own at all, so simplest fix: it doesn't
    # get one.
    git_marker = staged / ".git"
    if git_marker.is_dir():
        shutil.rmtree(git_marker)
    elif git_marker.exists():
        git_marker.unlink()

    # Every *.patch sitting directly in the platform's patch directory
    # applies to glib itself, here and now.
    glib_patches = sorted(patches_dir.glob("*.patch"))
    if not glib_patches:
        raise RuntimeError(f"no glib patches found in {patches_dir}")
    for patch_file in glib_patches:
        apply_unified_diff(patch_file.read_text(), staged)

    # Anything under a libffi/ subdirectory instead belongs to the libffi
    # subproject, which doesn't exist yet at this point -- Meson fetches it
    # during `meson setup`. Those get wired into libffi.wrap's own
    # diff_files, the same way deps/glib/subprojects/pcre2.wrap already
    # does for its Apple-platforms patch; see the module docstring for why
    # they can't be deferred the way the glib patches above are.
    #
    # Only PSP needs any: its MIPS target isn't in libffi's hardcoded
    # host_system allow-list, and its assembly needs fixing up for
    # Allegrex. Dreamcast's SH backend builds unmodified, so this is
    # skipped entirely there.
    libffi_patches_src = patches_dir / "libffi"
    if not libffi_patches_src.is_dir():
        return

    libffi_packagefiles = staged / "subprojects" / "packagefiles" / "libffi"
    libffi_packagefiles.mkdir(parents=True, exist_ok=True)
    patch_names = []
    for patch_file in sorted(libffi_patches_src.glob("*.patch")):
        shutil.copy(patch_file, libffi_packagefiles / patch_file.name)
        patch_names.append(f"libffi/{patch_file.name}")

    # Insert `diff_files = ...` as the last line of the [wrap-file]
    # section (i.e. right before the next `[section]` header, or at the
    # end if there isn't one).
    wrap_path = staged / "subprojects" / "libffi.wrap"
    lines = wrap_path.read_text().splitlines()
    insert_at = len(lines)
    for i, line in enumerate(lines):
        if i > 0 and line.startswith("[") and line.endswith("]"):
            insert_at = i
            break
    lines.insert(insert_at, f"diff_files = {','.join(patch_names)}")
    wrap_path.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("glib_source", type=Path)
    parser.add_argument("staged_dest", type=Path)
    parser.add_argument("patches_dir", type=Path)
    args = parser.parse_args()
    try:
        stage(args.glib_source, args.staged_dest, args.patches_dir)
    except Exception as e:
        print(f"stage_glib_for_target: {e}", file=sys.stderr)
        sys.exit(1)
