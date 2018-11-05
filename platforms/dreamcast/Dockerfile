# Dockerfile for generating a Fedora image with the full KallistiOS SDK installed so you can compile
# Dreamcast applications

FROM fedora:21
MAINTAINER kazade <kazade@gmail.com>

RUN yum clean all && yum -y install yum-plugin-ovl
RUN yum -y update && yum clean all && yum -y install automake make git hostname glibc-static bison elfutils flex glibc-devel binutils binutils-devel gdb tar bzip2 patch gcc gcc-c++ texinfo libjpeg-turbo-devel libpng-devel python cmake mpfr-devel gmp-devel libmpc-devel  openssl-devel python-devel python-setuptools libffi-devel genisoimage unzip python-pip && yum clean all && pip install -U pip
RUN mkdir -p /opt/toolchains/dc
RUN git clone git://git.code.sf.net/p/cadcdev/kallistios /opt/toolchains/dc/kos
RUN git clone --recursive git://git.code.sf.net/p/cadcdev/kos-ports /opt/toolchains/dc/kos-ports
#RUN sed -i 's/4.7.3/4.8.2/' /opt/toolchains/dc/kos/utils/dc-chain/download.sh
#RUN sed -i 's/4.7.3/4.8.2/' /opt/toolchains/dc/kos/utils/dc-chain/unpack.sh
#RUN sed -i 's/4.7.3/4.8.2/' /opt/toolchains/dc/kos/utils/dc-chain/Makefile
RUN cd /opt/toolchains/dc/kos/utils/dc-chain && sh ./download.sh && sh ./unpack.sh && make
RUN cp /opt/toolchains/dc/kos/doc/environ.sh.sample /opt/toolchains/dc/kos/environ.sh

# Enable C++ functionality which is disabled by default
RUN sed -i 's/-fno-rtti//' /opt/toolchains/dc/kos/environ_base.sh
RUN sed -i 's/-fno-exceptions//' /opt/toolchains/dc/kos/environ_base.sh
RUN sed -i 's/-fno-operator-names//' /opt/toolchains/dc/kos/environ_base.sh
RUN set -i 's/-g / /' /opt/toolchains/dc/kos/environ_base.sh

# Remove default optimisations, so it can be controlled by CMake or whatever
RUN sed -i 's/-O2//' /opt/toolchains/dc/kos/environ.sh

RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /etc/bash.bashrc
RUN ln -s /etc/bash.bashrc /root/.bashrc
RUN source /etc/bash.bashrc; echo ${KOS_BASE}
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos && make

RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libjpeg && make install clean
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libpng && make install clean
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && mkdir makeip && cd makeip && curl http://www.boob.co.uk/files/makeip.tar.gz > makeip.tar.gz && tar -xf makeip.tar.gz && rm makeip.tar.gz && gcc makeip.c -o makeip
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && git clone https://github.com/Kazade/img4dc.git && cd img4dc && mkdir build && cd build && cmake .. && make BUMP=9

# Add GLdc as a kos port
RUN mkdir -p /opt/toolchains/dc/kos-ports/GLdc/files
RUN echo $'\n\
TARGET = libGLdc.a\n\
OBJS = GL/draw.o GL/flush.o GL/framebuffer.o GL/immediate.o GL/lighting.o GL/state.o GL/texture.o GL/glu.o\n\
OBJS += GL/matrix.o GL/fog.o GL/error.o GL/clip.o containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o\n\
\n\
KOS_CFLAGS += -Iinclude -ffast-math -O3\n\
\n\
include ${KOS_PORTS}/scripts/lib.mk\n\
' > /opt/toolchains/dc/kos-ports/GLdc/files/KOSMakefile.mk

RUN echo $'\n\
PORTNAME = GLdc\n\
PORTVERSION = 1.0.0\n\
\n\
MAINTAINER = Luke Benstead <kazade@gmail.com>\n\
LICENSE = BSD 2-Clause "Simplified" License\n\
SHORT_DESC = Partial OpenGL 1.2 implementation for KOS\n\
\n\
DEPENDENCIES = \n\
\n\
GIT_REPOSITORY =    https://github.com/Kazade/GLdc.git\n\
\n\
TARGET =			libGLdc.a\n\
INSTALLED_HDRS =	include/gl.h include/glext.h include/glu.h include/glkos.h\n\
HDR_COMDIR =		GL\n\
\n\
KOS_DISTFILES =		KOSMakefile.mk\n\
\n\
include ${KOS_PORTS}/scripts/kos-ports.mk\n\
' > /opt/toolchains/dc/kos-ports/GLdc/Makefile

RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/GLdc && BUMP=1 make install clean

# Now install dcload-ip
RUN yum install -y elfutils-libelf-devel
RUN . /etc/bash.bashrc; git clone https://git.code.sf.net/p/cadcdev/dcload-ip /opt/toolchains/dc/dc-load-ip
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/dc-load-ip && make
RUN . /etc/bash.bashrc; mkdir -p /opt/toolchains/dc/bin && cp /opt/toolchains/dc/dc-load-ip/host-src/tool/dc-tool /opt/toolchains/dc/bin
