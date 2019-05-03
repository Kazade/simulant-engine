FROM openjdk:10-jdk
MAINTAINER Luke Benstead <kazade@gmail.com>

RUN mkdir -p /opt/android-sdk-linux && mkdir -p ~/.android && touch ~/.android/repositories.cfg
WORKDIR /opt

ENV ANDROID_HOME /opt/android-sdk-linux
ENV ANDROID_NDK /opt/android-ndk-linux
ENV ANDROID_NDK_HOME /opt/android-ndk-linux
ENV PATH ${PATH}:${ANDROID_HOME}/tools/bin:${ANDROID_HOME}/platform-tools:${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK_HOME}
ENV SDKMANAGER_OPTS "--add-modules java.se.ee"
RUN apt-get update && apt-get install -y --no-install-recommends \
	unzip wget cmake build-essential
RUN cd /opt/android-sdk-linux && \
	wget -q --output-document=sdk-tools.zip https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip && \
	unzip sdk-tools.zip && \
	rm -f sdk-tools.zip && \
	echo y | sdkmanager "build-tools;28.0.2" "platforms;android-28" && \
	echo y | sdkmanager "extras;android;m2repository" "extras;google;m2repository" "extras;google;google_play_services" && \
	sdkmanager "cmake;3.6.4111459"
RUN wget -q --output-document=android-ndk.zip https://dl.google.com/android/repository/android-ndk-r19c-linux-x86_64.zip && \
	unzip android-ndk.zip && \
	rm -f android-ndk.zip && \
	mv android-ndk-r19c /opt/android-ndk-linux
