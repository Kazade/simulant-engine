#!/usr/bin/env python
import os
import re
import sys
import shutil
import argparse
import subprocess
import multiprocessing

from StringIO import StringIO
from zipfile import ZipFile
from urllib import urlopen
from functools import partial

parser = argparse.ArgumentParser(description='Manage your Simulant-based project')
parser.add_argument('command', type=unicode, nargs=1, help='The command to run')
parser.add_argument('args', type=unicode, nargs="*")

VALID_TARGETS = [
    "android",
    "linux"
]

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(THIS_DIR, "build")
ANDROID_DIR = os.path.join(THIS_DIR, "android")
ASSETS_DIR = os.path.join(ANDROID_DIR, "assets")

ANDROID_JNI_DIR = os.path.join(ANDROID_DIR, "jni")
ANDROID_LIBS_DOWNLOAD_URL = "https://github.com/Kazade/kglt-android-libs/archive/master.zip"
ANDROID_SIMULANT_OUTPUT_DIR = os.path.join(ANDROID_JNI_DIR, "kglt")

PROJECT_NAME = "{PROJECT_NAME}"

SOURCE_DIR = os.path.join(THIS_DIR, PROJECT_NAME)
SCREENS_DIR = os.path.join(SOURCE_DIR, "screens")

SCREEN_TEMPLATE_H = """
#ifndef %(header_name_upper)s
#define %(header_name_upper)s

#include <kglt/kglt.h>

class %(name)s :
    public kglt::Screen<%(name)s> {

public:
    %(name)s(kglt::WindowBase& window):
        kglt::Screen<%(name)s>(window) {}

    void do_load();
};

#endif
"""

SCREEN_TEMPLATE_CPP = """
#include "%(header_name)s.h"

void %(name)s::do_load() {

}
"""

def _convert(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def update(args):
    args = args.args
    if not args:
        print("You must specify a target for update, valid targets are: %s" % (VALID_TARGETS,))
        return 1

    target = args[0]

    if target == "android":
        if os.path.exists(ANDROID_SIMULANT_OUTPUT_DIR):
            shutil.rmtree(ANDROID_SIMULANT_OUTPUT_DIR)

        print("Please wait, downloading libraries...")
        url = urlopen(ANDROID_LIBS_DOWNLOAD_URL)
        zipfile = ZipFile(StringIO(url.read()))
        print("Extracting...")
        zipfile.extractall(path=ANDROID_JNI_DIR)
        shutil.move(os.path.join(ANDROID_JNI_DIR, 'kglt-android-libs-master'), ANDROID_SIMULANT_OUTPUT_DIR)

        SIMULANT_ASSETS = os.path.join(ASSETS_DIR, "kglt")

        if os.path.exists(SIMULANT_ASSETS):
            os.unlink(SIMULANT_ASSETS)

        os.symlink(os.path.join(ANDROID_SIMULANT_OUTPUT_DIR, "assets"), SIMULANT_ASSETS)
        print("SIMULANT update complete")
        return 0
    else:
        print("No update command is available/needed for the %s target" % target)
        return 0

def update_version(define_name):
    version_file = os.path.join(SOURCE_DIR, "version.h")

    with open(version_file, "r") as f:
        lines = f.readlines()

    for i, line in enumerate(lines):
        if line.startswith("#define {}".format(define_name)):
            number = int(line.rsplit(" ", 1)[-1])
            number += 1

            lines[i] = line.replace(str(number - 1), str(number))
            break

    with open(version_file, "w") as f:
        f.writelines(lines)


def build(args, is_release=False):
    args = args.args
    if not args:
        print("You must specify a target for a %s build, valid targets are: %s" % (
            "release" if is_release else "debug",
            ", ".join(VALID_TARGETS))
        )
        return 1

    target = args[0]

    if not os.path.exists(BUILD_DIR):
        os.makedirs(BUILD_DIR)

    if target == "linux":
        update_version("LINUX_BUILD")

        os.chdir(BUILD_DIR)
        subprocess.check_call(["cmake", ".."])
        subprocess.check_call(["make", "-j", str(multiprocessing.cpu_count())])
    elif target == "android":
        update_version("ANDROID_BUILD")

        # Make sure SIMULANT is available
        if not os.path.exists(ANDROID_SIMULANT_OUTPUT_DIR):
            class Args:
                args.args = ["android"]
            update(Args())

        #Now, make sure we symlink the right libraries
        header_symlink = os.path.join(ANDROID_JNI_DIR, "include")
        if os.path.exists(header_symlink):
            os.unlink(header_symlink)

        libs_symlink = os.path.join(ANDROID_JNI_DIR, "libs")
        if os.path.exists(libs_symlink):
            os.unlink(libs_symlink)

        # Make sure the assets are available
        ASSET_DIR = os.path.join(ANDROID_DIR, "assets", PROJECT_NAME)
        if os.path.exists(ASSET_DIR):
            os.unlink(ASSET_DIR)

        os.symlink(os.path.join(ANDROID_SIMULANT_OUTPUT_DIR, "include"), header_symlink)
        os.symlink(os.path.join(ANDROID_SIMULANT_OUTPUT_DIR, "libs", "release" if is_release else "debug"), libs_symlink)
        os.symlink(os.path.join(THIS_DIR, PROJECT_NAME, "assets", PROJECT_NAME), ASSET_DIR)

        os.chdir(ANDROID_DIR)
        if is_release:
            # Make sure we clean up everything
            subprocess.check_call(["ant", "clean"])

            shutil.rmtree(os.path.join(ANDROID_DIR, "libs", "x86"), ignore_errors=True) # Libs should be rebuilt
            shutil.rmtree(os.path.join(ANDROID_DIR, "libs", "armeabi-v7a"), ignore_errors=True) # Libs should be rebuilt
            shutil.rmtree(os.path.join(ANDROID_DIR, "libs", "armeabi"), ignore_errors=True) # Libs should be rebuilt
            shutil.rmtree(os.path.join(ANDROID_DIR, "obj"), ignore_errors=True) # The bin folder will be recreated

            subprocess.check_call(["ndk-build"])
            subprocess.check_call(["ant", "release"])
        else:
            env = os.environ.copy()
            env["NDK_DEBUG"] = "1"
            subprocess.check_call(["ndk-build"], env=env)
            subprocess.check_call(["ant", "debug"])

def create(args):
    args = args.args
    if not args:
        print("You must specify a valid object, valid objects are: screen")
        return 1

    if len(args) != 2:
        print("You must specify an object and name")
        return 1

    obj, name = args

    if obj not in ("screen",):
        print("Invalid object name")
        return 1

    if obj == "screen":
        if not os.path.exists(SCREENS_DIR):
            print("Unable to find screens directory at %s, it appears you've customized the project so the create command will not work" % SCREENS_DIR)
            return 1


        header_name = _convert(name)

        header_file = os.path.join(SCREENS_DIR, header_name + ".h")
        source_file = os.path.join(SCREENS_DIR, header_name + ".cpp")

        if os.path.exists(header_file) or os.path.exists(source_file):
            print("Screen with the name '%s' already exists" % name)
            return 1

        with open(header_file, "wt") as header:
            header.write(SCREEN_TEMPLATE_H % dict(name=name, header_name_upper=header_name.upper().replace(".", "_")))

        with open(source_file, "wt") as source:
            source.write(SCREEN_TEMPLATE_CPP % dict(name=name, header_name=header_name))

        print("%s and %s created successfully" % (header_file, source_file))
        return 0

def main():
    args = parser.parse_args()

    COMMANDS = {
        "build": build,
        "debug": build,
        "update": update,
        "create": create,
        "release": lambda x: build(x, True)
    }

    command = args.command[0]

    if command not in COMMANDS:
        print("Unrecognized command: %s" % command)
        return 1

    return COMMANDS[command](args)

if __name__ == '__main__':
    sys.exit(main())
