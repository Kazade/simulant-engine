FROM fedora:36 as Build
MAINTAINER Luke Benstead <kazade@gmail.com>

RUN mkdir -p /opt/android-sdk-linux && mkdir -p ~/.android && touch ~/.android/repositories.cfg
WORKDIR /opt

ENV ANDROID_HOME /opt/android-sdk-linux
ENV ANDROID_NDK /opt/android-ndk-linux
ENV ANDROID_NDK_HOME /opt/android-ndk-linux
ENV PATH ${PATH}:${ANDROID_HOME}/tools/bin:${ANDROID_HOME}/platform-tools:${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK_HOME}:${ANDROID_HOME}/cmdline-tools/latest/bin
RUN dnf upgrade --refresh -y && dnf install -y unzip wget cmake python3 java-1.8.0-openjdk
RUN cd /opt/android-sdk-linux/ && \
	wget -q --output-document=sdk-tools.zip https://dl.google.com/android/repository/commandlinetools-linux-8512546_latest.zip && \
	unzip sdk-tools.zip && \
	rm -f sdk-tools.zip
RUN yes | /opt/android-sdk-linux/cmdline-tools/bin/sdkmanager --install "cmdline-tools;latest" --sdk_root=${ANDROID_HOME}
ENV JAVA_OPTS -XX:+IgnoreUnrecognizedVMOptions
RUN yes | sdkmanager --install "build-tools;29.0.3" "platforms;android-31"
RUN yes | sdkmanager --install "extras;android;m2repository" "extras;google;m2repository" "extras;google;google_play_services"
RUN yes | sdkmanager --install "cmake;3.6.4111459"
RUN cd /opt && wget -q --output-document=android-ndk.zip https://dl.google.com/android/repository/android-ndk-r25b-linux.zip && \
	unzip android-ndk.zip && \
	rm -f android-ndk.zip && \
	mv android-ndk-r25b android-ndk-linux
	
FROM fedora:36
ENV ANDROID_HOME /opt/android-sdk-linux
ENV ANDROID_NDK /opt/android-ndk-linux
ENV ANDROID_NDK_HOME /opt/android-ndk-linux
COPY --from=Build ${ANDROID_HOME} ${ANDROID_HOME}
COPY --from=Build ${ANDROID_NDK} ${ANDROID_NDK}
COPY --from=Build /root/.android /root/.android
RUN dnf install -y cmake make libpng genisoimage findutils git pkgconf-pkg-config libisofs java-1.8.0-openjdk

