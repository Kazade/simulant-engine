FROM fedora:37

RUN dnf install -y git make clang cmake bison flex llvm lld
RUN git clone --recursive https://github.com/XboxDev/nxdk.git /usr/src/nxdk
ENV NXDK_DIR=/usr/src/nxdk
RUN ls /usr/src/nxdk
RUN cd /usr/src/nxdk && make tools -j`nproc`
ARG buildparams
RUN eval $(./usr/src/nxdk/bin/activate -s); cd /usr/src/nxdk && make NXDK_ONLY=y $buildparams -j`nproc`

WORKDIR /usr/src/app
ENTRYPOINT ["/usr/src/nxdk/docker_entry.sh"]
