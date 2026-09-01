# Build pspbuild (https://github.com/ItsNoHax/pspbuild) in its own stage so
# the Rust toolchain doesn't end up in the final image - only the compiled
# binary is copied across below.
FROM alpine:3.24 AS pspbuild-builder

RUN apk add --no-cache rust cargo git build-base

RUN git clone --depth 1 https://github.com/ItsNoHax/pspbuild.git /pspbuild
WORKDIR /pspbuild
RUN cargo build --release

FROM pspdev/pspdev:latest
MAINTAINER kazade <kazade@gmail.com>

RUN apk add --no-cache python3 py3-pip

COPY --from=pspbuild-builder /pspbuild/target/release/pspbuild /usr/local/bin/pspbuild
