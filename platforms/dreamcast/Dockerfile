# Dockerfile for generating a Fedora image with the full KallistiOS SDK installed so you can compile
# Dreamcast applications

FROM fedora:30 as Build
MAINTAINER kazade <kazade@gmail.com>

RUN dnf -y update && dnf clean all && dnf -y install automake wget make git hostname glibc-static bison elfutils flex glibc-devel binutils binutils-devel gdb tar xz bzip2 patch gcc gcc-c++ texinfo libjpeg-turbo-devel libpng-devel python3 cmake mpfr-devel gmp-devel libmpc-devel  openssl-devel python-devel python-setuptools libffi-devel genisoimage unzip python-pip elfutils-libelf-devel wodim && dnf clean all && pip install -U pip
RUN mkdir -p /opt/toolchains/dc
RUN git clone https://github.com/KallistiOS/KallistiOS.git /opt/toolchains/dc/kos && BUMP=1
RUN git clone --recursive git://git.code.sf.net/p/cadcdev/kos-ports /opt/toolchains/dc/kos-ports

## Uncomment the below to enable a 5.2 toolchain

## Download update GCC and newlib diffs
#RUN cd /opt/toolchains/dc/kos/utils/dc-chain/patches; curl -O https://raw.githubusercontent.com/DC-SWAT/#DreamShell/master/sdk/toolchain/patches/gcc-5.2.0-kos.diff; cd -
#RUN cd /opt/toolchains/dc/kos/utils/dc-chain/patches; curl -O https://raw.githubusercontent.com/DC-SWAT/#DreamShell/master/sdk/toolchain/patches/newlib-2.2.0-kos.diff; cd -
#
## Custom SWAT/DreamShell patches which should probably be upstream??
#RUN cd /opt/toolchains/dc/kos/utils/dc-chain/patches; curl -O https://raw.githubusercontent.com/DC-SWAT/#DreamShell/master/sdk/toolchain/patches/kos.diff; cd -
#
#RUN cd /opt/toolchains/dc/kos && patch -p1 < /opt/toolchains/dc/kos/utils/dc-chain/patches/kos.diff
#

## Update newlib
#RUN sed -i 's/2.0.0/2.2.0/' /opt/toolchains/dc/kos/utils/dc-chain/download.sh
#RUN sed -i 's/2.0.0/2.2.0/' /opt/toolchains/dc/kos/utils/dc-chain/unpack.sh
#RUN sed -i 's/2.0.0/2.2.0/' /opt/toolchains/dc/kos/utils/dc-chain/Makefile
# Speed up compilation
RUN sed -i 's/# makejobs=-jn/makejobs=-j5/' /opt/toolchains/dc/kos/utils/dc-chain/Makefile

# Disable objc, and obj-c++
RUN sed -i 's/c,c++,objc,obj-c++/c,c++/' /opt/toolchains/dc/kos/utils/dc-chain/Makefile

RUN cd /opt/toolchains/dc/kos/utils/dc-chain && cp config.mk.stable.sample config.mk && sh ./download.sh && sh ./unpack.sh && make
RUN cp /opt/toolchains/dc/kos/doc/environ.sh.sample /opt/toolchains/dc/kos/environ.sh

# Enable frame pointer stack tracing stuff
RUN sed -i 's/-g -fno-builtin/-g -fno-builtin -DFRAME_POINTERS/' /opt/toolchains/dc/kos/environ_base.sh

# Enable C++ functionality which is disabled by default
RUN sed -i 's/-fno-rtti//' /opt/toolchains/dc/kos/environ_base.sh
RUN sed -i 's/-fno-exceptions//' /opt/toolchains/dc/kos/environ_base.sh
RUN sed -i 's/-fno-operator-names//' /opt/toolchains/dc/kos/environ_base.sh
RUN sed -i 's/-g / /' /opt/toolchains/dc/kos/environ_base.sh

# Remove default optimisations, so it can be controlled by CMake or whatever
RUN sed -i 's/-O2//' /opt/toolchains/dc/kos/environ.sh

RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /etc/bash.bashrc
RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /root/.bashrc
RUN sed -i 's/dcdis//g' /opt/toolchains/dc/kos/utils/Makefile
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos && make

RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libjpeg && make install clean
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libpng && make install clean
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils/makeip && make
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && git clone https://github.com/Kazade/img4dc.git && cd img4dc && mkdir build && cd build && cmake .. && make BUMP=9

# Add GLdc as a kos port
RUN mkdir -p /opt/toolchains/dc/kos-ports/GLdc/files && BUMP=2
RUN echo $'\n\
TARGET = libGLdc.a\n\
OBJS = GL/draw.o GL/flush.o GL/framebuffer.o GL/immediate.o GL/lighting.o GL/state.o GL/texture.o GL/glu.o\n\
OBJS += GL/matrix.o GL/fog.o GL/error.o GL/clip.o containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o\n\
\n\
KOS_CFLAGS += -Iinclude -ffast-math -Os\n\
\n\
include ${KOS_PORTS}/scripts/lib.mk\n\
' > /opt/toolchains/dc/kos-ports/GLdc/files/KOSMakefile.mk

RUN echo $'\n\
PORTNAME = GLdc\n\
PORTVERSION = 1.1.0\n\
\n\
MAINTAINER = Luke Benstead <kazade@gmail.com>\n\
LICENSE = BSD 2-Clause "Simplified" License\n\
SHORT_DESC = Partial OpenGL 1.2 implementation for KOS\n\
\n\
DEPENDENCIES = \n\
\n\
GIT_REPOSITORY =    https://gitlab.com/simulant/GLdc.git\n\
\n\
TARGET =			libGLdc.a\n\
INSTALLED_HDRS =	include/gl.h include/glext.h include/glu.h include/glkos.h\n\
HDR_COMDIR =		GL\n\
\n\
KOS_DISTFILES =		KOSMakefile.mk\n\
MAKE_TARGET =       build install\n\
\n\
include ${KOS_PORTS}/scripts/kos-ports.mk\n\
' > /opt/toolchains/dc/kos-ports/GLdc/Makefile

RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/GLdc && BUMP=1 make install clean

# Now install dcload-ip
RUN . /etc/bash.bashrc; git clone https://git.code.sf.net/p/cadcdev/dcload-ip /opt/toolchains/dc/dc-load-ip
RUN . /etc/bash.bashrc; cp /opt/toolchains/dc/dc-load-ip/example-src/dc4.x /opt/toolchains/dc/dc-load-ip/example-src/dc5.x
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/dc-load-ip && make
RUN . /etc/bash.bashrc; mkdir -p /opt/toolchains/dc/bin && cp /opt/toolchains/dc/dc-load-ip/host-src/tool/dc-tool /opt/toolchains/dc/bin

FROM fedora:30
COPY --from=Build /opt/toolchains /opt/toolchains
COPY --from=Build /etc/bash.bashrc /etc/bash.bashrc
COPY --from=Build /root/.bashrc /root/.bashrc
RUN dnf install -y cmake make libpng genisoimage

