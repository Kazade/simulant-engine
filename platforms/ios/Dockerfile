#
# The toolchain for cross-compiling Simulant for iOS: upstream clang (which
# can target iOS out of the box), plus Apple's linker (ld64) and binary tools
# (ar, ranlib, lipo, ...) from cctools-port, and the libraries they need.
#
# This image deliberately has NO iOS SDK in it. The SDK comes from Xcode and
# Apple's license doesn't allow redistributing it, so it mustn't end up in a
# public image. Mount or download one at build time instead, and point
# toolchains/iOS.cmake at it with IOS_SDK, e.g.
#
#   docker run -v /path/to/iPhoneOS.sdk:/sdk/iPhoneOS.sdk ...
#   cmake -DCMAKE_TOOLCHAIN_FILE=../../toolchains/iOS.cmake \
#         -DIOS_SDK=/sdk/iPhoneOS.sdk ../..
#
# See documentation/guides/ios.md for how to package an SDK from Xcode.
#
FROM fedora:44 AS build
MAINTAINER Luke Benstead <kazade@gmail.com>

# Pinned, so that rebuilding the image gives the same tools
ARG LIBTAPI_REF=fa9443738c1a18accef4244732ec6d6ee97a8133       # branch 1600.0.11.8
ARG LIBDISPATCH_REF=323b9b4e0ca05d6c56a0c2f2d7d8d47363e612b7   # branch main
ARG CCTOOLS_REF=904de2a71d4da6a9b30d2efaf912a10ddc7d9ddb       # branch 1030.6.3-ld64-956.6

ENV PREFIX=/opt/ios-toolchain

RUN dnf install -y \
        clang lld llvm cmake ninja-build make python3 git \
        autoconf automake libtool patch which libuuid-devel \
    && dnf clean all

ENV CC=clang
ENV CXX=clang++

# Apple's TAPI library, for reading the .tbd stubs that SDKs have instead of
# real libraries
RUN git clone https://github.com/tpoechtrager/apple-libtapi.git /tmp/libtapi \
    && cd /tmp/libtapi && git checkout ${LIBTAPI_REF} \
    && INSTALLPREFIX=${PREFIX} JOBS=$(nproc) ./build.sh \
    && ./install.sh \
    && rm -rf /tmp/libtapi

# libdispatch and the blocks runtime, which ld64 uses
RUN git clone https://github.com/tpoechtrager/apple-libdispatch.git /tmp/libdispatch \
    && cd /tmp/libdispatch && git checkout ${LIBDISPATCH_REF} \
    && cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${PREFIX} \
    && cmake --build build \
    && cmake --install build \
    && rm -rf /tmp/libdispatch

# ld64 and cctools. The target only decides the prefix of the tools' names:
# ld64 itself links any architecture, and clang assembles by itself.
RUN git clone https://github.com/tpoechtrager/cctools-port.git /tmp/cctools \
    && cd /tmp/cctools && git checkout ${CCTOOLS_REF} \
    && cd cctools \
    && ./configure \
        --prefix=${PREFIX} \
        --target=arm64-apple-darwin \
        --with-libtapi=${PREFIX} \
        --with-libdispatch=${PREFIX} \
        --with-libblocksruntime=${PREFIX} \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/cctools

# Unprefixed names, which is what toolchains/iOS.cmake uses
RUN cd ${PREFIX}/bin \
    && for tool in ld ar ranlib lipo libtool nm otool strip install_name_tool; do \
        ln -sf arm64-apple-darwin-${tool} ${tool}; \
    done \
    && ln -sf arm64-apple-darwin-ld ld64


FROM fedora:44
MAINTAINER Luke Benstead <kazade@gmail.com>

ENV IOS_TOOLCHAIN=/opt/ios-toolchain
COPY --from=build /opt/ios-toolchain /opt/ios-toolchain

# python3 is for the test generator, curl and xz for fetching an SDK, and
# rsync and zip for packaging the libraries
RUN dnf install -y \
        clang lld llvm cmake make python3 git curl xz tar rsync zip findutils libuuid \
    && dnf clean all

# ld64 needs libtapi and libdispatch at runtime
RUN echo "${IOS_TOOLCHAIN}/lib" > /etc/ld.so.conf.d/ios-toolchain.conf && ldconfig

ENV PATH=${PATH}:${IOS_TOOLCHAIN}/bin

# Fail the image build if the linker can't run
RUN ld64 -v
