FROM registry.fedoraproject.org/fedora:44
LABEL maintainer="kazade <kazade@gmail.com>"
RUN sudo dnf install -y mingw64-gcc-c++ mingw64-SDL2 mingw64-openal-soft mingw64-zlib git cmake make gcc-c++ python wine winetricks wine-pulseaudio xorg-x11-server-Xvfb xorg-x11-drv-dummy dpkg mesa-dri-drivers mingw32-nsis findutils git
ENV WINEPREFIX=/wine
RUN mkdir ${WINEPREFIX}
RUN adduser simulant
USER simulant
