FROM fedora:34
MAINTAINER Luke Benstead <kazade@gmail.com>

RUN mkdir -p /opt/android-sdk-linux && mkdir -p ~/.android && touch ~/.android/repositories.cfg
WORKDIR /opt

ENV ANDROID_HOME /opt/android-sdk-linux
ENV GRADLE_HOME /opt/gradle
ENV JAVA_HOME /etc/alternatives/jre

ENV PATH ${PATH}:${ANDROID_HOME}/cmdline-tools/latest/bin:${ANDROID_HOME}/platform-tools:${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK_HOME}:${GRADLE_HOME}/bin
RUN dnf install -y unzip wget cmake python3 java-11-openjdk-devel
RUN cd /opt/android-sdk-linux && \
	wget -q --output-document=sdk-tools.zip https://dl.google.com/android/repository/commandlinetools-linux-7302050_latest.zip && \
	unzip sdk-tools.zip && \
	rm -f sdk-tools.zip && \
	mkdir -p cmdline-tools-tmp && \
	mv cmdline-tools cmdline-tools-tmp/latest && \
	mv cmdline-tools-tmp cmdline-tools && \
	echo y | sdkmanager "build-tools;30.0.2" "platforms;android-28" && \
	echo y | sdkmanager "extras;android;m2repository" "extras;google;m2repository" "extras;google;google_play_services" && \
	echo y | sdkmanager "cmake;3.6.4111459" "ndk;21.1.6352462"
RUN wget -q --output-document=gradle.zip https://downloads.gradle-dn.com/distributions/gradle-7.0.2-bin.zip && \
    unzip gradle.zip && \
    rm -f gradle.zip && \
    mv gradle-7.0.2 /opt/gradle

