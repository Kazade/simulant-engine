FROM registry.fedoraproject.org/fedora-minimal:38
LABEL maintainer="kazade <kazade@gmail.com>"
RUN microdnf install -y mingw64-gcc-c++ mingw64-SDL2 mingw64-openal-soft mingw64-zlib git cmake make gcc-c++ python wine winetricks wine-pulseaudio xorg-x11-server-Xvfb xorg-x11-drv-dummy dpkg mesa-dri-drivers mingw32-nsis findutils git && microdnf clean all
RUN wine wineboot
RUN adduser simulant
USER simulant
