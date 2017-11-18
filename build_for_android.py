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
ANDROID_MK_PATH = os.path.join(OUTPUT_DIRECTORY, "Android.mk")
ANDROID_APP_MK_PATH = os.path.join(OUTPUT_DIRECTORY, "Application.mk")
ANDROID_MANIFEST_PATH = os.path.join(OUTPUT_DIRECTORY, "AndroidManifest.xml")

MAKE_FILE_DATA = """
include $(call all-subdir-makefiles)
""".strip()

APPLICATION_FILE_DATA = """
APP_BUILD_SCRIPT        := Android.mk
APP_PROJECT_PATH        := %s
NDK_TOOLCHAIN_VERSION   := clang
APP_GNUSTL_CPP_FEATURES := rtti exceptions
APP_CFLAGS              := %%s -DSIMULANT_GL_VERSION_2X
APP_STL                 := c++_shared
APP_PLATFORM            := android-18
APP_ABI                 := armeabi armeabi-v7a x86
APP_OPTIM               := debug
""".lstrip() % OUTPUT_DIRECTORY

LIBRARIES = [
    {
        "name": "sdl",
        "zip": "https://www.libsdl.org/release/SDL2-2.0.3.zip",
        "include": "sdl/include"
    },
    {
        "name": "openal",
        "repo": "https://github.com/Kazade/openal-soft.git",
        "include": "openal/OpenAL/include"
    },
    {
        "name": "tinyxml",
        "repo": "https://github.com/Kazade/tinyxml.git",
        "include": "tinyxml"
    }
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

    #First, symlink simulant into the .android folder (so that it's with all the others)
    simulant_link = os.path.join(OUTPUT_DIRECTORY, "simulant")
    if not os.path.exists(simulant_link):
        os.symlink(os.path.dirname(os.path.abspath("__file__")), simulant_link)

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

    if len(sys.argv) > 1 and sys.argv[1] == "gather_headers":
        gather_headers(sys.argv[2])
    else:
        os.chdir(OUTPUT_DIRECTORY)


        ANDROID_NDK = os.path.dirname(subprocess.check_output(["which", "ndk-build"]))

        command = "cmake -DCMAKE_TOOLCHAIN_FILE={0}/build/cmake/android.toolchain.cmake " \
        "-DCMAKE_BUILD_TYPE=Debug " \
        "-DANDROID_ABI=armeabi-v7a " \
        "-DANDROID_NDK={0} " \
        "-DANDROID_NATIVE_API_LEVEL=21 " \
        "-DANDROID_STL=c++_shared " \
        "..".format(ANDROID_NDK)

        subprocess.check_call(command.split())
        subprocess.check_call(["make"])

