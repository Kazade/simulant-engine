#!/usr/bin/env python3
import subprocess
import sys
import os

DOXYFILE_TEMPLATE = """
DOXYFILE_ENCODING      = UTF-8
PROJECT_NAME           = "Simulant"
PROJECT_NUMBER         = {version}
PROJECT_BRIEF          = "A portable game engine for Windows, OSX, Linux, Dreamcast, and PSP"
PROJECT_LOGO           = {logo_file}
OUTPUT_DIRECTORY       = {output_dir}
INPUT                  = {root_dir}
RECURSIVE              = YES
EXCLUDE                = {root_dir}/deps
HTML_STYLESHEET        = {stylesheet}
"""

if __name__ == '__main__':
    path = sys.argv[1]
    version = sys.argv[2]
    output = sys.argv[3]

    with open("/tmp/Doxyfile", "wt") as f:
        f.write(DOXYFILE_TEMPLATE.format(
            version="12.06",
            logo_file=os.path.join(path, "tools/simulant-icon-55.png"),
            root_dir=os.path.join(path, "simulant"),
            output_dir=os.path.join(output, version),
            stylesheet=os.path.join(path, "tools/doxygen.css")
        ))

    subprocess.check_call(["doxygen", "/tmp/Doxyfile"])
