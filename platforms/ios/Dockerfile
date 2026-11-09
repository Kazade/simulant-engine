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
# extract-ios-sdks (extract_sdks.py) extracts the SDKs from an Xcode .xip,
# e.g. into a mounted directory. See documentation/guides/ios.md.
#
# Build it with platforms/ios as the context:
#
#   docker build -t simulant-ios-toolchain platforms/ios
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

# python3 is for the test generator, curl and xz for fetching an SDK, rsync
# and zip for packaging the libraries, and openssl for reading the .p12
# certificates simulant-tools signs apps with
RUN dnf install -y \
        clang lld llvm cmake make python3 git curl xz tar rsync zip findutils libuuid openssl \
    && dnf clean all

# ld64 needs libtapi and libdispatch at runtime
RUN echo "${IOS_TOOLCHAIN}/lib" > /etc/ld.so.conf.d/ios-toolchain.conf && ldconfig

ENV PATH=${PATH}:${IOS_TOOLCHAIN}/bin

# Fail the image build if the linker can't run
RUN ld64 -v

# rcodesign (apple-codesign, MPL-2.0), for signing app bundles without a Mac:
# simulant-tools uses it to sign iOS apps with a .p12 certificate and a
# provisioning profile, and to ad-hoc sign simulator builds. Pinned, with the
# release's checksums.
ARG RCODESIGN_VERSION=0.29.0
RUN case "$(uname -m)" in \
        x86_64) sha=dbe85cedd8ee4217b64e9a0e4c2aef92ab8bcaaa41f20bde99781ff02e600002 ;; \
        aarch64) sha=4af92c87ddf52f5f2d1258a3b4e56c7dcb8f1b2468df744976c5f139e031961f ;; \
        *) echo "No rcodesign for $(uname -m)"; exit 1 ;; \
    esac \
    && name=apple-codesign-${RCODESIGN_VERSION}-$(uname -m)-unknown-linux-musl \
    && curl -fsSL -o /tmp/rcodesign.tar.gz \
        https://github.com/indygreg/apple-platform-rs/releases/download/apple-codesign/${RCODESIGN_VERSION}/${name}.tar.gz \
    && echo "${sha}  /tmp/rcodesign.tar.gz" | sha256sum -c - \
    && tar -xzf /tmp/rcodesign.tar.gz -C /tmp \
    && install -m 755 /tmp/${name}/rcodesign ${IOS_TOOLCHAIN}/bin/rcodesign \
    && rm -rf /tmp/rcodesign.tar.gz /tmp/${name} \
    && rcodesign --version

# For extracting the SDKs from an Xcode .xip (see the top of this file)
COPY extract_sdks.py ${IOS_TOOLCHAIN}/bin/extract-ios-sdks
RUN chmod 755 ${IOS_TOOLCHAIN}/bin/extract-ios-sdks
