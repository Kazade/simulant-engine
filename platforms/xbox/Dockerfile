FROM fedora:37

RUN dnf install -y git make clang cmake bison flex llvm lld
RUN git clone --recursive https://github.com/XboxDev/nxdk.git /usr/src/nxdk
RUN cd /usr/src/nxdk/lib/pdclib && git remote add floats https://github.com/antonic901/nxdk-pdclib.git
RUN cd /usr/src/nxdk/lib/pdclib && git fetch floats && git checkout a52d41cf21391d3c489e8ae0f1504c26d05d4a57
ENV NXDK_DIR=/usr/src/nxdk
RUN cd /usr/src/nxdk && git remote add rtti https://github.com/antonic901/nxdk.git
RUN cd /usr/src/nxdk && git fetch rtti && git checkout rtti/thrimbor_exceptions
RUN cd /usr/src/nxdk && make tools -j`nproc`
ARG buildparams
RUN eval $(./usr/src/nxdk/bin/activate -s); cd /usr/src/nxdk && make NXDK_ONLY=y $buildparams -j`nproc`

WORKDIR /usr/src/app
ENTRYPOINT ["/usr/src/nxdk/docker_entry.sh"]
