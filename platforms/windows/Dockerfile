FROM registry.fedoraproject.org/fedora-minimal:33
MAINTAINER kazade <kazade@gmail.com>
RUN microdnf install -y mingw64-gcc-c++ mingw64-SDL2 mingw64-openal-soft mingw64-zlib cmake make gcc-c++ python wine winetricks wine-pulseaudio pulseaudio pulseaudio-utils xorg-x11-server-Xvfb xorg-x11-drv-dummy dpkg mesa-dri-drivers mingw32-nsis findutils && microdnf clean all
RUN wine wineboot
RUN adduser simulant
USER simulant
