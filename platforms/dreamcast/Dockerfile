# Dockerfile for generating a Fedora image with the full KallistiOS SDK installed so you can compile
# Dreamcast applications

FROM fedora:21
MAINTAINER kazade <kazade@gmail.com>

RUN yum -y update && yum clean all && yum -y install automake make git hostname glibc-static bison elfutils flex glibc-devel binutils gdb tar bzip2 patch gcc gcc-c++ texinfo libjpeg-turbo-devel libpng-devel python cmake mpfr-devel gmp-devel libmpc-devel && yum clean all
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

# Remove any custom frame-pointer overrides (just use default)
RUN sed -i 's/-fomit-frame-pointer//' /opt/toolchains/dc/kos/environ.sh

# Remove default optimisations, so it can be controlled by CMake or whatever
RUN sed -i 's/-O2//' /opt/toolchains/dc/kos/environ.sh 

RUN echo -e "\nsource /opt/toolchains/dc/kos/environ.sh" >> /etc/bash.bashrc
RUN source /etc/bash.bashrc; echo ${KOS_BASE}
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos && make

# Switch the libGL port to my fork
RUN sed -i 's/git:\/\/git.code.sf.net\/p\/cadcdev\/libgl/https:\/\/github.com\/Kazade\/libgl.git/' /opt/toolchains/dc/kos-ports/libGL/Makefile
 
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libjpeg && make install clean
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libpng && make install clean
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && mkdir makeip && cd makeip && curl http://www.boob.co.uk/files/makeip.tar.gz > makeip.tar.gz && tar -xf makeip.tar.gz && rm makeip.tar.gz && gcc makeip.c -o makeip
RUN . /etc/bash.bashrc; yum -y install genisoimage unzip
RUN . /etc/bash.bashrc; cd /opt/toolchains/dc/kos/utils && git clone https://github.com/Kazade/img4dc.git && cd img4dc && mkdir build && cd build && cmake .. && make BUMP=7
RUN source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/libGL && BUMP=18 make install clean 
