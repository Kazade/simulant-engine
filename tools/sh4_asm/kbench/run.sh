#!/bin/bash
# Build kbench against a kernel asm file and run it on a Dreamcast over
# serial, with the kazade/dreamcast-sdk docker image. Prints steady-state
# cycles per vertex for each kernel.
#
# usage: tools/sh4_asm/kbench/run.sh [asm file]
#   (default: simulant/renderers/pvr/pvr_lighting_sh4.s)
# env:   DC_SERIAL  serial device (default /dev/ttyUSB0)
#        DC_TOOL    host path of dc-tool-ser (default /usr/local/sbin/dc-tool-ser)
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
asm=$(realpath "${1:-$root/simulant/renderers/pvr/pvr_lighting_sh4.s}")
serial=${DC_SERIAL:-/dev/ttyUSB0}
dctool=${DC_TOOL:-/usr/local/sbin/dc-tool-ser}

cp "$asm" "$here/kernels.s"
defs=""
grep -q "_pvr_light_point_sh4:" "$here/kernels.s" && defs="-DHAVE_POINT_ASM"
grep -q "add     #56, r2" "$here/kernels.s" && defs="$defs -DPACK32"
docker run --rm -v "$root:$root:Z" -w "$here" kazade/dreamcast-sdk \
    /bin/bash -lc "source /etc/bash.bashrc; \
        kos-c++ -O3 -ffast-math -mfsrra -mfsca -ffp-contract=fast -I$root/deps/sh4zam/include -c -o pointc.o pointc.cpp && \
        kos-cc -O2 $defs -c -o kbench.o kbench.c && \
        kos-c++ -o kbench.elf kbench.o pointc.o kernels.s"
docker run --rm --device="$serial" -v "$root:$root:Z" -v "$dctool:/dc-tool-ser:ro" \
    -w "$here" kazade/dreamcast-sdk \
    /bin/bash -lc "source /etc/bash.bashrc; timeout 60 /dc-tool-ser -t $serial -b 1562500 -x $here/kbench.elf" \
    | grep -a '\[kbench\]'
