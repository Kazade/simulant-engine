FROM fedora:36
MAINTAINER kazade <kazade@gmail.com>

RUN dnf install -y SDL2 openal-soft mesa-libGL libstdc++ pulseaudio pulseaudio-utils cmake make gcc-c++ SDL2-devel openal-soft-devel zlib-devel mesa-libGL-devel docker vala ninja-build python3-pip
# deps/glib (glib 2.90) requires meson >= 1.4.0; Fedora 36 only ships
# 0.62, so it comes from pip rather than dnf. See cmake/BuildGLib.cmake.
RUN pip3 install --upgrade meson
RUN adduser simulant
USER simulant
