#!/usr/bin/env python3
"""cgen: generates a C-linkage wrapper (libsimulant-c) around Simulant's C++
public API by scanning it with libclang.

Usage:
    python3 tools/cgen/cgen.py [options]

Requires the `libclang` Python package (`pip install libclang`) and a clang
install (used to auto-detect the resource dir with builtin headers).

See tools/cgen/README.md for the naming convention and the current
limitations of what can be wrapped.
"""
import argparse
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from cgen_lib import clangutil, config, log
from cgen_lib.emitter import Emitter
from cgen_lib.renames import apply_renames
from cgen_lib.scanner import Scanner
from cgen_lib.vapi_emitter import VapiEmitter


def _import_clang():
    try:
        import clang.cindex as cindex
    except ImportError:
        log.error("could not import 'clang.cindex'. Install it with: pip install libclang")
        sys.exit(1)
    return cindex


def parse_args(argv):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--input", action="append", default=None,
                   help="Header to scan (relative to the repo root, or absolute). "
                        "Repeatable. Defaults to a small curated set of headers.")
    p.add_argument("--include-dir", "-I", action="append", default=[],
                   help="Extra include directory for parsing. Repeatable.")
    p.add_argument("--define", "-D", action="append", default=[],
                   help="Extra preprocessor define for parsing. Repeatable.")
    p.add_argument("--clang-arg", action="append", default=[],
                   help="Extra raw argument passed straight through to clang.")
    p.add_argument("--std", default="c++20", help="C++ standard to parse with (default: c++20)")
    p.add_argument("--out-dir", default=config.DEFAULT_OUT_DIR,
                   help="Where to write generated .h/.cpp files (default: ports/c/generated)")
    p.add_argument("--repo-root", default=config.REPO_ROOT,
                   help="Simulant repo root (default: auto-detected)")
    p.add_argument("--ignore-file", default=config.DEFAULT_IGNORE_FILE,
                   help="JSON file listing fully-qualified C++ names to always skip")
    p.add_argument("--rename-file", default=config.DEFAULT_RENAME_FILE,
                   help="JSON file mapping default generated C names to hand-chosen ones")
    p.add_argument("--libclang", default=None, help="Explicit path to libclang.so, if needed")
    p.add_argument("-v", "--verbose", action="count", default=0,
                   help="Increase verbosity (-v info, -vv debug)")
    p.add_argument("-q", "--quiet", action="store_true", help="Only print errors")
    p.add_argument("--strict", action="store_true",
                   help="Exit non-zero if any class/method/function had to be skipped")
    p.add_argument("--vapi", default=config.DEFAULT_VAPI_PATH,
                   help="Also write a Vala .vapi binding for the generated C API to this path "
                        f"(default: {config.DEFAULT_VAPI_PATH})")
    p.add_argument("--no-vapi", action="store_true", help="Skip generating the .vapi")
    p.add_argument("--vapi-namespace", default="Smlt",
                   help="Vala namespace for --vapi output (default: Smlt)")
    return p.parse_args(argv)


def build_clang_args(args):
    clang_args = ["-x", "c++", f"-std={args.std}"]
    resource_dir = clangutil.clang_resource_dir()
    if resource_dir:
        clang_args += ["-resource-dir", resource_dir]

    include_dirs = [args.repo_root, os.path.join(args.repo_root, "simulant")]
    include_dirs += config.DEFAULT_EXTRA_INCLUDE_DIRS
    include_dirs += clangutil.sdl2_include_dirs()
    include_dirs += args.include_dir
    for d in include_dirs:
        clang_args.append(f"-I{d}")

    for d in args.define:
        clang_args.append(f"-D{d}")

    clang_args += args.clang_arg
    return clang_args


def load_ignore_set(path):
    if not path or not os.path.exists(path):
        return set()
    with open(path) as f:
        data = json.load(f)
    return set(data.get("ignore", []))


def load_rename_map(path):
    if not path or not os.path.exists(path):
        return {}
    with open(path) as f:
        data = json.load(f)
    return dict(data.get("renames", {}))


def resolve_inputs(args):
    inputs = args.input or config.DEFAULT_INPUT_HEADERS
    resolved = []
    for header in inputs:
        candidate = header if os.path.isabs(header) else os.path.join(args.repo_root, header)
        if not os.path.exists(candidate):
            log.error(f"input header not found: {candidate}")
            sys.exit(1)
        resolved.append(candidate)
    return resolved


def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)

    if args.quiet:
        log.set_level(log.QUIET)
    elif args.verbose >= 2:
        log.set_level(log.DEBUG)
    elif args.verbose == 1:
        log.set_level(log.VERBOSE)
    else:
        log.set_level(log.NORMAL)

    cindex = _import_clang()
    libclang_path = args.libclang or clangutil.find_system_libclang()
    if libclang_path:
        log.info(f"using libclang: {libclang_path}")
        cindex.Config.set_library_file(libclang_path)

    headers = resolve_inputs(args)
    clang_args = build_clang_args(args)
    log.info(f"clang args: {' '.join(clang_args)}")
    log.status(f"scanning {len(headers)} header(s): {', '.join(os.path.relpath(h, args.repo_root) for h in headers)}")

    # Parse every requested header as one translation unit so cross-header
    # type references (e.g. Stage's constructor taking a Scene*) resolve
    # regardless of which file happens to declare which class first. Using
    # absolute paths here sidesteps any ambiguity from quote-include
    # relative-to-current-file resolution.
    umbrella_src = "\n".join(f'#include "{h}"' for h in headers)

    with tempfile.NamedTemporaryFile(mode="w", suffix=".hpp", delete=False) as tmp:
        tmp.write(umbrella_src)
        tmp_path = tmp.name

    try:
        index = cindex.Index.create()
        tu = index.parse(tmp_path, args=clang_args,
                         options=cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    finally:
        os.unlink(tmp_path)

    fatal = False
    for diag in tu.diagnostics:
        msg = f"{diag.location}: {diag.spelling}"
        if diag.severity >= cindex.Diagnostic.Error:
            log.error(f"clang: {msg}")
            fatal = True
        else:
            log.debug(f"clang: {msg}")

    if fatal:
        log.error("parsing failed, see above; generated output (if any) may be incomplete")

    ignore_set = load_ignore_set(args.ignore_file)
    scanner = Scanner(config.default_path_filter, ignore_set)
    scanner.collect(tu.cursor)
    ir = scanner.extract(tu.cursor)

    rename_map = load_rename_map(args.rename_file)
    apply_renames(ir, rename_map)

    emitter = Emitter(args.out_dir, args.repo_root)
    emitter.write_all(ir)

    if args.vapi and not args.no_vapi:
        os.makedirs(os.path.dirname(os.path.abspath(args.vapi)), exist_ok=True)
        VapiEmitter(namespace=args.vapi_namespace).write(ir, args.vapi)

    log.status(f"done: {log.warning_count()} warning(s), {log.error_count()} error(s)")

    if log.error_count() > 0:
        return 1
    if args.strict and log.warning_count() > 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
