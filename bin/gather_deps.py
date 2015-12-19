#!/usr/bin/env python

"""
    This script downloads the dependencies of KGLT so they
    can be built for the target platform by CMake. This is
    mainly for Dreamcast and Android builds.
"""

import os
import sys
import tempfile
import shutil
import subprocess

from urllib import urlopen
from zipfile import ZipFile
from cStringIO import StringIO

def _download_and_extract_zip(zip_file, target_dir):
    url = urlopen(zip_file)
    zipfile = ZipFile(StringIO(url.read()))
    folder_name = zipfile.namelist()[0].rstrip("/")
    zipfile.extractall(path=os.path.dirname(target_dir))

    final_loc = os.path.join(os.path.dirname(target_dir), folder_name)
    shutil.move(
        final_loc,
        target_dir
    )

    return os.path.join(target_dir, folder_name)


DEPENDENCIES = (
    (
        "kazbase",
        "https://github.com/Kazade/kazbase/archive/master.zip"
    ),
)


if __name__ == '__main__':
    toolchain_file = os.path.abspath(sys.argv[1])

    output_dir = tempfile.mkdtemp()

    target_name = os.path.splitext(os.path.split(toolchain_file)[-1])[0].strip()

    if not os.path.exists("./deps"):
        os.makedirs("./deps")

    print "Preparing dependencies for toolchain:", target_name
    for name, zip_file in DEPENDENCIES:
        print "Processing dependency:", name
        library_folder = _download_and_extract_zip(zip_file, output_dir)

        build_dir = os.path.join(library_folder, "build")

        os.makedirs(build_dir)

        subprocess.check_call([
            "cmake", "-DCMAKE_TOOLCHAIN_FILE={}".format(toolchain_file), ".."
        ], cwd=build_dir)

        subprocess.check_call([
            "make"
        ], cwd=build_dir)
