#!/usr/bin/env python3
#
# Extracts the iOS SDKs (iPhoneOS and iPhoneSimulator) from an Xcode .xip,
# without a Mac and without unpacking the rest of Xcode, and makes them
# linkable with cctools-port's ld64.
#
#   extract_sdks.py Xcode_27.2.xip /opt/ios-sdks
#       -> /opt/ios-sdks/iPhoneOS27.2.sdk, /opt/ios-sdks/iPhoneSimulator27.2.sdk
#
#   extract_sdks.py --fix /path/to/iPhoneOS.sdk ...
#       Only makes SDKs from elsewhere (e.g. copied from Xcode on a Mac)
#       linkable: see make_sdk_linkable.
#
# Used by Simulant's CI, and installed in the iOS toolchain image as
# extract-ios-sdks, which simulant-tools runs. Only needs Python 3's standard
# library.
#
# The SDKs are Apple's and can't be redistributed: keep them out of
# repositories, public CI artifacts and Docker images.
#

import json
import os
import re
import shutil
import struct
import sys
import zlib


class _ChunkReader(object):
    """A file-like reader over a generator of byte strings"""

    def __init__(self, chunks):
        self._chunks = chunks
        self._buffer = bytearray()

    def read(self, size):
        while len(self._buffer) < size:
            chunk = next(self._chunks, None)
            if chunk is None:
                break
            self._buffer += chunk

        data = bytes(self._buffer[:size])
        del self._buffer[:size]
        return data

    def skip(self, size):
        while size > 0:
            # Throw away what's buffered, then whole chunks, without keeping
            # more than one in memory
            if self._buffer:
                n = min(size, len(self._buffer))
                del self._buffer[:n]
                size -= n
                continue

            chunk = next(self._chunks, None)
            if chunk is None:
                return
            self._buffer += chunk


def _xip_payload(xip):
    """
    Yields the decompressed contents of an Xcode .xip, which is a cpio
    archive of Xcode.app.

    A .xip is a xar archive. Its "Content" file is a pbzx stream: chunks of
    xz compressed data, one after another.
    """
    import lzma
    import xml.etree.ElementTree as ET

    with open(xip, "rb") as f:
        header = f.read(28)
        if header[:4] != b"xar!":
            raise ValueError("%s isn't a .xip (xar) archive" % xip)

        header_size, _, toc_length = struct.unpack(">HHQ", header[4:16])
        f.seek(header_size)
        toc = ET.fromstring(zlib.decompress(f.read(toc_length)))
        heap_start = header_size + toc_length

        content = None
        for entry in toc.iter("file"):
            if entry.findtext("name") == "Content":
                content = entry.find("data")
                break

        if content is None:
            raise ValueError("%s has no Xcode payload" % xip)

        f.seek(heap_start + int(content.findtext("offset")))
        remaining = int(content.findtext("length"))

        def read(n):
            nonlocal remaining
            data = f.read(n)
            if len(data) != n:
                raise ValueError("%s is truncated" % xip)
            remaining -= n
            return data

        if read(4) != b"pbzx":
            raise ValueError("%s's payload isn't pbzx compressed" % xip)

        flags = struct.unpack(">Q", read(8))[0]
        while flags & 0x01000000 and remaining > 0:
            flags, length = struct.unpack(">QQ", read(16))
            chunk = read(length)
            # Chunks that wouldn't compress are stored as they are
            if chunk[:6] == b"\xfd7zXZ\x00":
                yield lzma.decompress(chunk, format=lzma.FORMAT_XZ)
            else:
                yield chunk


# Apple's cpio archives store a hard-linked file's content once. Its other
# names (later entries with the same device, inode and size) get this
# instead, padded with zeros to the file's size.
_CPIO_LINK_PLACEHOLDER = b"NULLcanary"

_S_IFMT, _S_IFDIR, _S_IFLNK, _S_IFREG = 0o170000, 0o040000, 0o120000, 0o100000


def _cpio_entries(reader):
    """
    Yields (name, dev, ino, mode, size) for each entry of an odc cpio archive.
    The caller must read or skip each entry's size bytes before the next.
    """
    while True:
        header = reader.read(76)
        if len(header) < 76:
            raise ValueError("The archive ended unexpectedly")
        if header[:6] != b"070707":
            raise ValueError("Unexpected data in the archive (not odc cpio)")

        dev = int(header[6:12], 8)
        ino = int(header[12:18], 8)
        mode = int(header[18:24], 8)
        name_size = int(header[59:65], 8)
        size = int(header[65:76], 8)
        name = reader.read(name_size)[:-1].decode("utf-8", "surrogateescape")

        if name == "TRAILER!!!":
            return

        yield (name[2:] if name.startswith("./") else name), dev, ino, mode, size


# Architectures in the SDKs' .tbd stubs that cctools-port's ld64 (through
# libtapi 1600, the newest there is) doesn't know, and fails to link with
# ("unknown architecture"). Each is always listed alongside one it does know.
_UNKNOWN_TBD_TARGETS = ("arm64e.x1-ios",)


def make_sdk_linkable(sdk):
    """
    Removes the architectures ld64 doesn't know from the SDK's .tbd stubs.
    Apps link against the stubs' arm64 or arm64e entries, which stay. Returns
    how many stubs were changed.
    """
    targets = re.compile(r"^(\s*(?:- )?targets:\s*\[)([^\]]*)(\].*)$", re.M)

    def fix(match):
        kept = [
            t.strip()
            for t in match.group(2).split(",")
            if t.strip() and t.strip() not in _UNKNOWN_TBD_TARGETS
        ]
        return match.group(1) + " " + ", ".join(kept) + " " + match.group(3)

    changed = 0
    for root, _, files in os.walk(sdk):
        for name in files:
            path = os.path.join(root, name)
            if not name.endswith(".tbd") or os.path.islink(path):
                continue

            with open(path, encoding="utf-8", errors="surrogateescape") as f:
                text = f.read()

            if not any(t in text for t in _UNKNOWN_TBD_TARGETS):
                continue

            fixed = targets.sub(fix, text)
            with open(path, "w", encoding="utf-8", errors="surrogateescape") as f:
                f.write(fixed)
            changed += 1

    return changed



def extract_ios_sdks(xip, output):
    """
    Extracts the iPhoneOS and iPhoneSimulator SDKs from an Xcode .xip into
    output, as iPhoneOS<version>.sdk and iPhoneSimulator<version>.sdk, without
    unpacking the rest of Xcode (which is many times bigger).

    The SDKs are Apple's and can't be redistributed: keep them out of
    repositories and Docker images.
    """
    output = os.path.abspath(os.path.expanduser(output))
    os.makedirs(output, exist_ok=True)

    # Unpacked here first, then moved into place once complete
    staging = os.path.join(output, ".extracting")
    if os.path.exists(staging):
        shutil.rmtree(staging)
    os.makedirs(staging)
    staging_real = os.path.realpath(staging)

    # Content needed for hard links, from files outside the SDKs
    link_cache = os.path.join(staging, ".links")
    os.makedirs(link_cache)

    wanted = re.compile(
        r"^(?:[^/]+/)*Xcode[^/]*\.app/Contents/Developer/Platforms/"
        r"(iPhoneOS|iPhoneSimulator)\.platform/Developer/SDKs(?:/(.*))?$"
    )

    def safe_destination(name):
        # The archive is downloaded data, so it mustn't be able to write
        # anywhere outside the staging directory
        if name.startswith("/") or ".." in name.split("/"):
            raise ValueError("Unsafe path in the archive: %s" % name)
        path = os.path.normpath(os.path.join(staging, name))
        parent = os.path.realpath(os.path.dirname(path))
        if parent != staging_real and not parent.startswith(staging_real + os.sep):
            raise ValueError("Path in the archive leads outside: %s" % name)
        return path

    def write_entry(reader, path, data, left, mode):
        with open(path, "wb") as out:
            out.write(data)
            while left > 0:
                chunk = reader.read(min(left, 1024 * 1024))
                if not chunk:
                    raise ValueError("The archive ended unexpectedly")
                out.write(chunk)
                left -= len(chunk)
        os.chmod(path, mode & 0o755 | 0o600)

    marker_size = len(_CPIO_LINK_PLACEHOLDER)

    # First pass: which of the SDKs' files are hard links, whose content
    # comes from another entry (which may not be in the SDKs at all)
    print("Reading %s (1 of 2)..." % xip)
    needed = set()
    reader = _ChunkReader(_xip_payload(xip))
    for name, dev, ino, mode, size in _cpio_entries(reader):
        if (mode & _S_IFMT) == _S_IFREG and size >= marker_size and wanted.match(name):
            if reader.read(marker_size) == _CPIO_LINK_PLACEHOLDER:
                needed.add((dev, ino, size))
            reader.skip(size - marker_size)
        else:
            reader.skip(size)

    # Second pass: unpack the SDKs, keeping the content of linked files
    print("Extracting the iOS SDKs (2 of 2)...")
    content = {}   # (dev, ino, size) -> a file with the content
    pending = []   # Hard links: (path, key, mode)
    found = 0
    reader = _ChunkReader(_xip_payload(xip))
    for name, dev, ino, mode, size in _cpio_entries(reader):
        kind = mode & _S_IFMT
        key = (dev, ino, size)
        match = wanted.match(name)

        if not match:
            if kind == _S_IFREG and key in needed and key not in content:
                data = reader.read(min(size, marker_size))
                if data == _CPIO_LINK_PLACEHOLDER:
                    reader.skip(size - len(data))
                else:
                    path = os.path.join(link_cache, str(len(content)))
                    write_entry(reader, path, data, size - len(data), mode)
                    content[key] = path
            else:
                reader.skip(size)
            continue

        platform_name, rest = match.group(1), match.group(2) or ""
        path = safe_destination(os.path.join(platform_name, rest))
        found += 1

        if kind == _S_IFDIR:
            os.makedirs(path, exist_ok=True)
            reader.skip(size)
        elif kind == _S_IFLNK:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            target = reader.read(size).decode("utf-8", "surrogateescape")
            if os.path.lexists(path):
                os.unlink(path)
            os.symlink(target, path)
        elif kind == _S_IFREG:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            data = reader.read(min(size, marker_size))
            if key in needed and data == _CPIO_LINK_PLACEHOLDER:
                reader.skip(size - len(data))
                pending.append((path, key, mode))
                continue

            write_entry(reader, path, data, size - len(data), mode)
            if key in needed and key not in content:
                content[key] = path
        else:
            reader.skip(size)

    missing = []
    for path, key, mode in pending:
        if key not in content:
            missing.append(path)
            continue
        shutil.copyfile(content[key], path)
        os.chmod(path, mode & 0o755 | 0o600)

    if missing:
        raise ValueError(
            "%d hard-linked files in the SDKs had no content in the archive, "
            "e.g. %s" % (len(missing), missing[0])
        )

    print("  %d SDK entries, %d of them hard links" % (found, len(pending)))

    # Each platform's SDKs directory has the SDK, and a symlink to it with
    # the version in its name. Keep the SDK, named for its version.
    extracted = []
    for platform_name in ("iPhoneOS", "iPhoneSimulator"):
        sdks = os.path.join(staging, platform_name)
        sdk = os.path.join(sdks, platform_name + ".sdk")
        if not os.path.isdir(sdk) or os.path.islink(sdk):
            print("Couldn't find %s.sdk in %s" % (platform_name, xip))
            continue

        with open(os.path.join(sdk, "SDKSettings.json")) as f:
            version = json.load(f)["Version"]

        changed = make_sdk_linkable(sdk)
        if changed:
            print(
                "  Removed %s from %d of %s's .tbd stubs (cctools-port can't link with it)"
                % (", ".join(_UNKNOWN_TBD_TARGETS), changed, platform_name)
            )

        destination = os.path.join(output, "%s%s.sdk" % (platform_name, version))
        if os.path.exists(destination):
            shutil.rmtree(destination)
        shutil.move(sdk, destination)
        extracted.append(destination)

    shutil.rmtree(staging)

    for path in extracted:
        print("Extracted %s" % path)

    return extracted


def main(argv):
    if len(argv) >= 2 and argv[0] == "--fix":
        for sdk in argv[1:]:
            changed = make_sdk_linkable(sdk)
            print(
                "Removed %s from %d of %s's .tbd stubs"
                % (", ".join(_UNKNOWN_TBD_TARGETS), changed, sdk)
            )
        return 0

    if len(argv) != 2 or argv[0].startswith("-"):
        print("Usage: %s XIP OUTPUT_DIRECTORY" % os.path.basename(sys.argv[0]))
        print("       %s --fix SDK [SDK ...]" % os.path.basename(sys.argv[0]))
        return 2

    try:
        extracted = extract_ios_sdks(argv[0], argv[1])
    except (OSError, ValueError, EOFError) as e:
        print("Couldn't extract the SDKs: %s" % e)
        return 1

    return 0 if len(extracted) == 2 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
