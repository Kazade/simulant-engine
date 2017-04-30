# Dockerfile for generating a Fedora image with the full KallistiOS SDK installed so you can compiled
# Dreamcast applications

FROM fedora
MAINTAINER kazade <kazade@gmail.com>

# RUN dnf -y update && dnf clean all
RUN dnf -y install git bzip2 patch gcc texinfo libjpeg-turbo-devel libpng-devel python && dnf clean all
RUN mkdir -p /opt/toolchains/dc
RUN git clone git://git.code.sf.net/p/cadcdev/kallistios /opt/toolchains/dc/kos
RUN git clone --recursive git://git.code.sf.net/p/cadcdev/kos-ports /opt/toolchains/dc/kos-ports
RUN cd /opt/toolchains/dc/kos/utils/dc-chain && sh ./download.sh && sh ./unpack.sh && make
RUN cp /opt/toolchains/dc/kos/doc/environ.sh.sample /opt/toolchains/dc/kos/environ.sh
RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /etc/bash.bashrc
RUN source /etc/bash.bashrc; echo ${KOS_BASE}
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos && make
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libGL && make install clean
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libjpeg && make install clean
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libpng && make install clean
RUN dnf -y install cmake

