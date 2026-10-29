FROM fedora:43 AS build
LABEL maintainer="Luke Benstead <kazade@gmail.com>"

RUN mkdir -p /opt/android-sdk-linux && mkdir -p ~/.android && touch ~/.android/repositories.cfg
WORKDIR /opt

ENV ANDROID_HOME=/opt/android-sdk-linux
ENV GRADLE_HOME=/opt/gradle
ENV PATH=${PATH}:${ANDROID_HOME}/platform-tools:${ANDROID_HOME}/cmdline-tools/latest/bin:${GRADLE_HOME}/bin
RUN dnf upgrade --refresh -y && dnf install -y unzip wget java-25-openjdk-devel
ENV JAVA_HOME=/usr/lib/jvm/java-25-openjdk
RUN cd /opt/android-sdk-linux/ && \
	wget -q --output-document=sdk-tools.zip https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip && \
	unzip sdk-tools.zip && \
	rm -f sdk-tools.zip
RUN yes | /opt/android-sdk-linux/cmdline-tools/bin/sdkmanager --install "cmdline-tools;latest" --sdk_root=${ANDROID_HOME}
ENV JAVA_OPTS=-XX:+IgnoreUnrecognizedVMOptions

# Only what building needs. (The emulator isn't, as simulant-tools runs its
# own outside the image, and Gradle gets libraries from Google's Maven
# repository rather than the old "extras" ones.)
RUN yes | sdkmanager --install \
	"build-tools;36.0.0" "platforms;android-36" "ndk;27.3.13750724" "cmake;3.22.1" "platform-tools"

RUN cd /tmp && wget -q --output-document=gradle.zip https://downloads.gradle.org/distributions/gradle-8.12-bin.zip && \
    unzip gradle.zip && \
    rm -f gradle.zip && \
    mv gradle-8.12 ${GRADLE_HOME}

# The permissions are set here rather than in the final image: there, it'd
# be a layer of its own holding a second copy of every file it touched (the
# whole SDK). COPY keeps them.
RUN chmod -R a+rwX ${ANDROID_HOME} ${GRADLE_HOME} /root/.android

FROM fedora:43
ENV ANDROID_HOME=/opt/android-sdk-linux
ENV ANDROID_NDK=/opt/android-sdk-linux/ndk/27.3.13750724
ENV ANDROID_NDK_HOME=/opt/android-sdk-linux/ndk/27.3.13750724
ENV JAVA_HOME=/usr/lib/jvm/java-25-openjdk
ENV GRADLE_HOME=/opt/gradle
ENV PATH=${PATH}:${ANDROID_HOME}/platform-tools:${ANDROID_NDK_HOME}:${ANDROID_HOME}/cmdline-tools/latest/bin:${GRADLE_HOME}/bin
RUN dnf install -y cmake make libpng genisoimage findutils git pkgconf-pkg-config libisofs java-25-openjdk-devel && \
	dnf clean all

# The NDK is inside the SDK, so it comes with it
COPY --from=build ${ANDROID_HOME} ${ANDROID_HOME}
COPY --from=build ${GRADLE_HOME} ${GRADLE_HOME}
COPY --from=build /root/.android /root/.android
RUN mkdir -p /root/.gradle && chmod 777 /root /root/.gradle

ENV HOME=/root
ENV ANDROID_USER_HOME=/root/.android
ENV GRADLE_USER_HOME=/root/.gradle
