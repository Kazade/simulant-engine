# Dockerfile for generating a Fedora image with the full KallistiOS SDK installed so you can compiled
# Dreamcast applications

FROM fedora
MAINTAINER kazade <kazade@gmail.com>

# RUN dnf -y update && dnf clean all
RUN dnf -y install git bzip2 patch gcc texinfo && dnf clean all
RUN DIR=$(mktemp -d -t) && cd ${DIR} && \
    git clone git://git.code.sf.net/p/cadcdev/kallistios && \
    git clone --recursive git://git.code.sf.net/p/cadcdev/kos-ports && \
    cd ${DIR}/kallistios/utils/dc-chain && \
    sh ./download.sh && \ 
    sh ./unpack.sh && \
    make && \
    cp ${DIR}/kallistios/doc/environ.sh.sample /etc/profile.d/dreamcast.sh && \
    source /etc/profile.d/dreamcast.sh && \
    cd ${DIR}/kallistios/ && make

