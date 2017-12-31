#!/usr/bin/env python

# This is a script to bundle and upload the compiled Dreamcast
# binaries to Google Cloud Storage.

# This is for use on Travis which frustratingly doesn't allow access to the
# checkout directory from inside the Docker container.
import os
import argparse
import subprocess
import sys

from zipfile import ZipFile


HEADER_EXTENSIONS = (".h", ".hpp")
LIBRARY_EXTENSIONS = (
    ".lib", ".dylib", ".dll", ".so", ".a"
)


def gather_files(source_folder, extensions):
    output = []

    for root, dirs, files in os.walk(source_folder):
        for filename in files:
            if os.path.splitext(filename)[-1] in extensions:
                path = os.path.join(root, filename)
                output.append(path)

    return output


def run(options):
    header_folder = os.path.abspath(options.header_folder)
    folder_name = "simulant-{}-{}".format(options.build_target, options.build_type)
    lib_files = gather_files(options.library_folder, LIBRARY_EXTENSIONS)
    header_files = gather_files(header_folder, HEADER_EXTENSIONS)

    zip_filename = os.path.join(options.target_folder, folder_name + ".zip")

    zipf = ZipFile(zip_filename, "w")

    for header in header_files:
        arcname = "{}/include/simulant{}".format(
            folder_name,
            header[len(header_folder):]
        )
        print("Adding %s to %s" % (header, arcname))
        zipf.write(header, arcname)

    for lib in lib_files:
        arcname = "{}/lib/{}/{}".format(
            folder_name,
            options.build_type,
            os.path.split(lib)[-1]
        )
        print("Adding %s to %s" % (lib, arcname))
        zipf.write(lib, arcname)

    zipf.close()

    if not options.package_only:
        echo = subprocess.Popen(
            ("echo", "-e", "{}\n{}\n{}\n".format(options.access_key, options.access_secret, options.project_id)),
            stdout=subprocess.PIPE
        )

        # Authenticate with gsutil
        command = ["gsutil", "config", "-a"]
        subprocess.check_call(command, stdin=echo.stdout)
        echo.wait()

        # Upload the file
        gs_url = "gs://{}".format(options.bucket)
        command = [
            "gsutil", "mv", zip_filename,
            gs_url
        ]
        subprocess.check_call(command)

        # Make it public
        gs_url = "{}/{}".format(gs_url, os.path.split(zip_filename)[-1])
        command = [
            "gsutil", "acl", "ch", "-u", "AllUsers:R",
            gs_url
        ]
        subprocess.check_call(command)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Upload to GCS")
    parser.add_argument("--build-type", dest="build_type", required=True)
    parser.add_argument("--build-target", dest="build_target", required=True)

    # This is where to find the header files
    parser.add_argument("--header-folder", dest="header_folder", required=True)

    # This is the directory we search for library files
    parser.add_argument("--lib-folder", dest="library_folder", required=True)

    # This is the folder where the zip is generated
    parser.add_argument("--target-folder", dest="target_folder", required=True)

    # Upload controls
    parser.add_argument("--bucket")
    parser.add_argument("--access-key")
    parser.add_argument("--access-secret")
    parser.add_argument("--project-id")
    parser.add_argument("--package-only", action="store_true")

    options = parser.parse_args()
    sys.exit(run(options))
