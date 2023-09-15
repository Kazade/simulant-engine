FROM fedora:36 as Build
MAINTAINER Luke Benstead <kazade@gmail.com>

RUN mkdir -p /opt/android-sdk-linux && mkdir -p ~/.android && touch ~/.android/repositories.cfg
WORKDIR /opt

ENV ANDROID_HOME /opt/android-sdk-linux
ENV ANDROID_NDK /opt/android-ndk-linux
ENV ANDROID_NDK_HOME /opt/android-ndk-linux
ENV GRADLE_HOME /opt/gradle
ENV PATH ${PATH}:${ANDROID_HOME}/tools/bin:${ANDROID_HOME}/platform-tools:${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK_HOME}:${ANDROID_HOME}/cmdline-tools/latest/bin:${GRADLE_HOME}/bin
RUN dnf upgrade --refresh -y && dnf install -y unzip wget cmake python3 java-latest-openjdk
RUN cd /opt/android-sdk-linux/ && \
	wget -q --output-document=sdk-tools.zip https://dl.google.com/android/repository/commandlinetools-linux-9477386_latest.zip && \
	unzip sdk-tools.zip && \
	rm -f sdk-tools.zip
RUN yes | /opt/android-sdk-linux/cmdline-tools/bin/sdkmanager --install "cmdline-tools;latest" --sdk_root=${ANDROID_HOME}
ENV JAVA_OPTS -XX:+IgnoreUnrecognizedVMOptions
RUN yes | sdkmanager --install "build-tools;33.0.2" "platforms;android-33" "ndk;25.2.9519653"
RUN yes | sdkmanager --install "extras;android;m2repository" "extras;google;m2repository" "extras;google;google_play_services"
RUN yes | sdkmanager --install "cmake;3.22.1"
RUN cd /tmp && wget -q --output-document=gradle.zip https://downloads.gradle.org/distributions/gradle-8.3-bin.zip && \
    unzip gradle.zip && \
    rm -f gradle.zip && \
    mv gradle-8.3 ${GRADLE_HOME}
    
FROM fedora:36
ENV ANDROID_HOME /opt/android-sdk-linux
ENV ANDROID_NDK /opt/android-sdk-linux/ndk/25.2.9519653
ENV ANDROID_NDK_HOME /opt/android-sdk-linux/ndk/25.2.9519653
ENV GRADLE_HOME /opt/gradle
ENV PATH ${PATH}:${ANDROID_HOME}/tools/bin:${ANDROID_HOME}/platform-tools:${PATH}:${ANDROID_HOME}/tools:${ANDROID_NDK_HOME}:${ANDROID_HOME}/cmdline-tools/latest/bin:${GRADLE_HOME}/bin
COPY --from=Build ${ANDROID_HOME} ${ANDROID_HOME}
COPY --from=Build ${ANDROID_NDK} ${ANDROID_NDK}
COPY --from=Build ${GRADLE_HOME} ${GRADLE_HOME}
COPY --from=Build /root/.android /root/.android
RUN dnf install -y cmake make libpng genisoimage findutils git pkgconf-pkg-config libisofs java-latest-openjdk

