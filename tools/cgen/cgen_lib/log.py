"""Tiny leveled logger. We deliberately avoid the stdlib `logging` module's
global configuration surface since this is a small standalone CLI tool and
we want dead-simple, predictable verbosity flags (-q / -v / -vv).
"""
import sys

QUIET = 0
NORMAL = 1
VERBOSE = 2
DEBUG = 3

_level = NORMAL
_warning_count = 0
_error_count = 0


def set_level(level: int):
    global _level
    _level = level


def get_level() -> int:
    return _level


def warning_count() -> int:
    return _warning_count


def error_count() -> int:
    return _error_count


def _emit(stream, prefix, msg):
    stream.write(f"{prefix} {msg}\n")


def debug(msg: str):
    if _level >= DEBUG:
        _emit(sys.stdout, "[debug]", msg)


def info(msg: str):
    if _level >= VERBOSE:
        _emit(sys.stdout, "[info]", msg)


def status(msg: str):
    """Always shown unless -q, but below warning severity."""
    if _level >= NORMAL:
        _emit(sys.stdout, "[cgen]", msg)


def warning(msg: str):
    global _warning_count
    _warning_count += 1
    if _level >= NORMAL:
        _emit(sys.stderr, "[warning]", msg)


def error(msg: str):
    global _error_count
    _error_count += 1
    _emit(sys.stderr, "[error]", msg)
