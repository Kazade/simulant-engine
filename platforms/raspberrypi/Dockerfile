FROM debian:bookworm-slim
MAINTAINER kazade <kazade@gmail.com>

RUN dpkg --add-architecture armhf
RUN apt-get update
RUN apt-get install -y libsdl2-dev:armhf libopenal-dev:armhf libc6-dev:armhf gcc-11-arm-linux-gnueabihf g++-11-arm-linux-gnueabihf cmake
