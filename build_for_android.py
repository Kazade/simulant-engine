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
APP_CFLAGS              := %s
APP_STL                 := c++_shared
APP_PLATFORM            := android-18
APP_ABI                 := armeabi armeabi-v7a x86
APP_OPTIM               := %s
""".lstrip()

LIBRARIES = [
    {
        "name": "pcre",
        "repo": "https://github.com/Kazade/pcre.git",
        "include": "pcre"
    },
    {
        "name": "kazmath",
        "repo": "https://github.com/Kazade/kazmath.git",
        "include": "kazmath"
    },
    {
        "name": "kaztimer",
        "repo": "https://github.com/Kazade/kaztimer.git",
        "include": "kaztimer"
    },
    {
        "name": "kazbase",
        "repo": "https://github.com/Kazade/kazbase.git",
        "include": "kazbase"
    },
    {
        "name": "sdl",
        "zip": "https://www.libsdl.org/release/SDL2-2.0.3.zip",
        "include": "sdl/include"
    },
    {
        "name": "soil",
        "repo": "https://github.com/Kazade/soil.git",
        "include": "soil/src"
    },
    {
        "name": "openal",
        "repo": "https://github.com/Kazade/openal-soft.git",
        "include": "openal/OpenAL/include"
    },
    {
        "name": "freetype2",
        "repo": "https://github.com/Kazade/freetype2.git",
        "include": "freetype2/include"
    },
    {
        "name": "lua",
        "repo": "https://github.com/Kazade/lua.git",
        "include": "lua/src"
    },
    {
        "name": "tinyxml",
        "repo": "https://github.com/Kazade/tinyxml.git",
        "include": "tinyxml"
    }
]

BUILD_TYPES = ("debug", "release")


from os.path import join, exists

def prepare_tree(output_dir):
    # First, let's make the directories we require

    dirs = os.listdir(output_dir)

    if "output" not in dirs:
        os.makedirs(join(output_dir, "output"))

    if "dependencies" not in dirs:
        os.makedirs(join(output_dir, "dependencies"))

    if "build" not in dirs:
        os.makedirs(join(output_dir, "build"))

    # Now symlink KGLT to the right place
    symlinks = (
        ("kglt", join(output_dir, "build", "kglt")),
        ("submodules", join(output_dir, "build", "submodules"))

    )

    this_dir = os.path.dirname(os.path.abspath("__file__"))
    for target, link_name in symlinks:
        if not exists(link_name):
            os.symlink(join(this_dir, target), join(this_dir, link_name))


def download_dependencies(root):
    deps_dir = join(root, "dependencies")

    downloaded = []

    for library in LIBRARIES:
        library_output_dir = join(deps_dir, library["name"])

        if exists(library_output_dir):
            continue

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

        downloaded.append(library["name"])

    return downloaded

def build_dependencies(root, to_build, build_type):
    deps_dir = join(root, "dependencies")

    if not to_build:
        to_build = [ x["name"] for x in LIBRARIES ]

    for library in to_build:
        print("Building: %s" % library)
        library_dir = join(deps_dir, library)

        # If there exists an application.mk in the library
        # then remove it
        application_mk = join(library_dir, "Application.mk")
        if exists(application_mk):
            os.remove(application_mk)

        # Generate a new one for this release
        with open(application_mk, "wt") as f:
            f.write(APPLICATION_FILE_DATA % (library_dir, '', build_type))

        subprocess.check_call(
            [ "NDK_PROJECT_PATH=.", "ndk-build" ],
            cwd=library_dir
        )


def run():
    prepare_tree(OUTPUT_DIRECTORY)
    downloaded = download_dependencies(OUTPUT_DIRECTORY)

    if downloaded or "--build-deps" in sys.argv:
        for build_type in BUILD_TYPES:
            build_dependencies(OUTPUT_DIRECTORY, downloaded, build_type)

    return 0

    build()

    return 0

if __name__ == '__main__':
    sys.exit(run())
