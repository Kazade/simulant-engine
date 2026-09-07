"""Default configuration for the generator. Everything here can be
overridden from the command line; these are just sane starting points so
`python3 tools/cgen/cgen.py` scans the whole public API out of the box.
Unsupported constructs (templates, STL containers, ...) are skipped with a
warning rather than aborting the run -- see tools/cgen/README.md.
"""
import os

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
CGEN_DIR = os.path.dirname(THIS_DIR)
REPO_ROOT = os.path.dirname(os.path.dirname(CGEN_DIR))

DEFAULT_OUT_DIR = os.path.join(REPO_ROOT, "ports", "c", "generated")
DEFAULT_VAPI_PATH = os.path.join(REPO_ROOT, "ports", "vala", "simulant-c.vapi")

DEFAULT_IGNORE_FILE = os.path.join(CGEN_DIR, "ignore.json")
DEFAULT_RENAME_FILE = os.path.join(CGEN_DIR, "renames.json")

# The umbrella header pulls in essentially the whole public API (nodes,
# application, scenes, ...). Narrow this with --input if you only care
# about a subset.
DEFAULT_INPUT_HEADERS = [
    "simulant/simulant.h",
]

# Extra -I directories needed by third-party headers pulled in transitively
# by the default input above (SDL2 is discovered separately via
# pkg-config/sdl2-config, see clangutil.sdl2_include_dirs()).
DEFAULT_EXTRA_INCLUDE_DIRS = [
    os.path.join(REPO_ROOT, "deps", "kazsignal", "include"),
    os.path.join(REPO_ROOT, "deps", "kfs", "include"),
    os.path.join(REPO_ROOT, "simulant", "deps", "bounce", "include"),
]


def default_path_filter(path: str) -> bool:
    """True if `path` is considered part of the Simulant codebase we want
    bindings for (as opposed to vendored third-party code or system
    headers). Used to decide which class/enum *definitions* get registered.
    """
    norm = os.path.normpath(path)
    parts = norm.split(os.sep)
    if "simulant" not in parts:
        return False
    if "deps" in parts:
        return False
    return True
