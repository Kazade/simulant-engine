#!/bin/env python

import os
import sys
import subprocess
import shutil
import multiprocessing

from StringIO import StringIO
from zipfile import ZipFile
from urllib import urlopen

CWD = os.getcwd()
OUTPUT_DIRECTORY = os.path.join(CWD, ".android")

LIBRARIES = [
    {
        "name": "sdl",
        "zip": "https://www.libsdl.org/release/SDL2-2.0.4.zip",
        "include": "sdl/include"
    },
    {
        "name": "openal",
        "repo": "https://github.com/Kazade/openal-soft.git",
        "include": "openal/OpenAL/include"
    },
]

def gather_headers(output_dir):
    file_list = []
    for library in LIBRARIES + [ {"name": "simulant", "include": "simulant/simulant"}]:
        library_include_root = os.path.join(OUTPUT_DIRECTORY, library["include"])
        for root, subFolders, files in os.walk(library_include_root):
            for f in files:
                if f.endswith(".h"):
                    path = os.path.join(root,f)
                    output = os.path.join(output_dir, library["name"] + path.replace(library_include_root, ""))

                    file_list.append((path, output))

    for src, dest in file_list:
        folder = os.path.dirname(dest)
        if not os.path.exists(folder):
            os.makedirs(folder)
        shutil.copy(src, dest)

if __name__ == "__main__":
    if not os.path.exists(OUTPUT_DIRECTORY):
        os.mkdir(OUTPUT_DIRECTORY)

    includes = []
    for library in LIBRARIES:

        if "include" in library:
            includes.append(library["include"])

        library_output_dir = os.path.join(OUTPUT_DIRECTORY, library["name"])
        if os.path.exists(library_output_dir):
            if "precompile" in library:
                library["precompile"]()
            continue
        else:
            print "Downloading %s" % library["name"]

        if "repo" in library:
            subprocess.check_call([
                "git", "clone", library["repo"], library_output_dir
            ])
        elif "zip" in library:
            url = urlopen(library["zip"])
            zipfile = ZipFile(StringIO(url.read()))

            folder_name = zipfile.namelist()[0].rstrip("/")
            zipfile.extractall(path=os.path.dirname(library_output_dir))
            shutil.move(
                os.path.join(os.path.dirname(library_output_dir), folder_name),
                library_output_dir
            )
        else:
            raise ValueError("Unable to handle this library")

        if "precompile" in library:
            library["precompile"]()

