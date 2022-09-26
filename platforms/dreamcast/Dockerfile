# Dockerfile for generating a Fedora image with the full KallistiOS SDK installed so you can compile
# Dreamcast applications

FROM fedora:30 as Build
MAINTAINER kazade <kazade@gmail.com>

RUN dnf -y update && dnf clean all && dnf -y install automake wget make git hostname glibc-static bison elfutils flex glibc-devel binutils binutils-devel gdb tar xz bzip2 patch gcc gcc-c++ texinfo libjpeg-turbo-devel libpng-devel python3 cmake mpfr-devel gmp-devel libmpc-devel meson openssl-devel python-devel python-setuptools libffi-devel genisoimage unzip python-pip elfutils-libelf-devel wodim && dnf clean all && pip install -U pip
RUN mkdir -p /opt/toolchains/dc
RUN git clone https://github.com/KallistiOS/KallistiOS.git /opt/toolchains/dc/kos
RUN cd /opt/toolchains/dc/kos && git fetch && git checkout be7878a32ae07f5a62fb2ba68c6e85a9d824a111 && cd -
RUN git clone --recursive https://github.com/KallistiOS/kos-ports.git /opt/toolchains/dc/kos-ports

# Speed up compilation
RUN sed -i 's/# makejobs=-jn/makejobs=-j5/' /opt/toolchains/dc/kos/utils/dc-chain/Makefile

# Disable objc, and obj-c++
RUN sed -i 's/c,c++,objc,obj-c++/c,c++/' /opt/toolchains/dc/kos/utils/dc-chain/Makefile

# Build the toolchain
RUN cd /opt/toolchains/dc/kos/utils/dc-chain && cp config.mk.testing.sample config.mk && sh ./download.sh && sh ./unpack.sh && make

# Copy the sample config
RUN cp /opt/toolchains/dc/kos/doc/environ.sh.sample /opt/toolchains/dc/kos/environ.sh

# Create a debug config
RUN cp /opt/toolchains/dc/kos/doc/environ.sh.sample /opt/toolchains/dc/kos/environ-debug.sh
RUN sed -i '$ i\export KOS_CFLAGS=\"${KOS_CFLAGS} -DKOS_DEBUG=1\"' /opt/toolchains/dc/kos/environ-debug.sh
RUN cat /opt/toolchains/dc/kos/environ-debug.sh

# Build debug KOS
RUN source /opt/toolchains/dc/kos/environ-debug.sh; cd /opt/toolchains/dc/kos && make
RUN mkdir -p /opt/toolchains/dc/kos/lib/dreamcast/debug
RUN mv /opt/toolchains/dc/kos/lib/dreamcast/*.a /opt/toolchains/dc/kos/lib/dreamcast/debug
RUN ls -l /opt/toolchains/dc/kos/lib/dreamcast/debug

RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /etc/bash.bashrc
RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /root/.bashrc

# Build KOS
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos && make

# FIXME: Restore when kos-ports is fixed
# RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libjpeg && make install clean
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libpng && make install clean
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils/makeip && make
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && git clone https://github.com/Kazade/img4dc.git && cd img4dc && mkdir build && cd build && cmake .. && make
RUN dnf -y install libisofs-devel
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && git clone https://gitlab.com/simulant/mkdcdisc.git && cd mkdcdisc && mkdir build && cd build && meson .. && ninja

FROM fedora:30
COPY --from=Build /opt/toolchains /opt/toolchains
COPY --from=Build /etc/bash.bashrc /etc/bash.bashrc
COPY --from=Build /root/.bashrc /root/.bashrc
COPY --from=Build /opt/toolchains/dc/kos/utils/mkdcdisc/build/mkdcdisc /usr/bin/mkdcdisc
RUN dnf install -y cmake make libpng genisoimage findutils git pkgconf-pkg-config libisofs

