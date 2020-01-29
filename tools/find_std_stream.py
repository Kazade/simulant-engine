#!/usr/bin/env python

"""
    Recursively finds usages of STL streams
"""

import os
import sys


TOKENS = (
    "include <iostream>",
    "include <fstream>",
    "include <sstream>",
    "std::ifstream",
    "std::ostream",
    "std::stringstream",
)


def scan_file(full_path):
    count = 0
    if not os.path.exists(full_path):
        return count

    with open(full_path, "r", errors='ignore') as f:
        data = f.read()

        if any(tok in data for tok in TOKENS):
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
            elif full_path.endswith(".cpp") or full_path.endswith(".h"):
                recurse.count += scan_file(full_path)
    recurse.count = 0

    recurse(os.path.abspath(path))
    if recurse.count:
        print("Found %s usages of std streams" % recurse.count)
    else:
        print("No usages of std streams found")

    sys.exit(recurse.count)

