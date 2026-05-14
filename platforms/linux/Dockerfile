FROM fedora:36
MAINTAINER kazade <kazade@gmail.com>

RUN dnf install -y SDL2 openal-soft mesa-libGL libstdc++ pulseaudio pulseaudio-utils cmake make gcc-c++ SDL2-devel openal-soft-devel zlib-devel mesa-libGL-devel docker
RUN adduser simulant
USER simulant
