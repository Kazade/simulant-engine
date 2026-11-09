#!/bin/bash
#
# Installs the iOS libraries built by build_libraries.sh into a Simulant
# project's libraries/ directory, laid out as `simulant update` would (and
# as the CI's ios-*-clang.zip files are):
#
#   <project>/libraries/ios-arm64-clang/include/simulant/...
#   <project>/libraries/ios-arm64-clang/lib/{debug,release}/libsimulant.a
#   <project>/libraries/ios-simulator-clang/...       (the same)
#
# Until the CI builds them (it needs the iOS SDKs, see
# documentation/guides/ios.md), this is how projects get them.
#
# Usage: install_libraries.sh <project directory>
#
# Run from the directory build_libraries.sh was run in. Whichever of Debug
# and Release have been built are installed.
#

set -euo pipefail

PROJECT=${1:?Usage: $0 <project directory>}
SOURCE_DIR=$(cd "$(dirname "$0")/../.." && pwd)

if [ ! -f "${PROJECT}/simulant.json" ]; then
    echo "${PROJECT} isn't a Simulant project (there's no simulant.json)"
    exit 1
fi

installed=0
for target in ios-arm64-clang ios-simulator-clang; do
    destination=${PROJECT}/libraries/${target}

    for type in debug release; do
        lib=${target}/${type}/simulant/libsimulant.a
        if [ -f "${lib}" ]; then
            mkdir -p "${destination}/lib/${type}"
            cp "${lib}" "${destination}/lib/${type}/"
            echo "Installed ${lib}"
            installed=1
        fi
    done

    if [ -d "${destination}/lib" ]; then
        rm -rf "${destination}/include"
        mkdir -p "${destination}/include"
        rsync -am -L --include='*.h' --include='*.hpp' --include='*.inc' \
            -f 'hide,! */' "${SOURCE_DIR}/simulant" "${destination}/include/"
    fi
done

if [ "${installed}" = 0 ]; then
    echo "Nothing to install: run build_libraries.sh first"
    exit 1
fi
