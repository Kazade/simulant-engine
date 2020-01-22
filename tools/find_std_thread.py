#!/usr/bin/env python

"""
    Recursively finds usages of std::thread, or includes
    for <thread> or <mutex> in C++ headers
    and source files. Returns the count.
"""

import os
import sys


def scan_file(full_path):
    count = 0
    if not os.path.exists(full_path):
        return count

    with open(full_path, "r") as f:
        data = f.read()

        if ("include <thread>" in data or
            "include <mutex>" in data or
            "std::thread" in data or
            "std::mutex" in data or
            "std::async" in data
        ):
            print(full_path)
            count += 1
    return count


if __name__ == '__main__':
    try:
        path = sys.argv[1]
    except IndexError:
        print("Please specify a path")
        sys.exit(-1)

    def recurse(folder):
        for name in os.listdir(folder):
            full_path = os.path.realpath(os.path.join(folder, name))
            if os.path.isdir(full_path):
                recurse(full_path)
            else:
                recurse.count += scan_file(full_path)
    recurse.count = 0

    recurse(os.path.abspath(path))
    if recurse.count:
        print("Found %s usages of std thread" % recurse.count)
    else:
        print("No usages of std thread found")

    sys.exit(recurse.count)

