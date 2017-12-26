#!/usr/bin/env python

# This is a script to bundle and upload the compiled Dreamcast
# binaries to Google Cloud Storage.

# This is for use on Travis which frustratingly doesn't allow access to the
# checkout directory from inside the Docker container.

import argparse
import subprocess
import shutil


def run(options):



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Upload to GCS")
    parser.add_argument("--bucket", dest="bucket", action="store_const")
    parser.add_argument("--package-only", dest="package_only", action="store_true")

    args = parser.parse_args()

    sys.exit(run())
