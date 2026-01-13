FROM fedora:37 AS builder

RUN dnf install -y git make clang cmake bison flex llvm
RUN git clone --recursive https://github.com/XboxDev/nxdk.git /usr/src/nxdk
ENV NXDK_DIR=/usr/src/nxdk
RUN ls /usr/src/nxdk
RUN cd /usr/src/nxdk && make tools -j`nproc`
ARG buildparams
RUN eval $(./usr/src/nxdk/bin/activate -s); cd /usr/src/nxdk && make NXDK_ONLY=y $buildparams -j`nproc`


FROM fedora:37

COPY --from=builder /usr/src/nxdk/ /usr/src/nxdk/
ENV NXDK_DIR=/usr/src/nxdk

WORKDIR /usr/src/app

ENTRYPOINT ["/usr/src/nxdk/docker_entry.sh"]
