#!/bin/bash

rm -rf openal || true
mkdir openal
cd openal

git clone https://github.com/google/oboe.git
cd oboe
git checkout 1.7.0
cd -

git clone https://github.com/kcat/openal-soft.git
cd openal-soft
git checkout 1.23.1
cd -

mkdir -p openal-soft/build/armeabi-v7a && cd openal-soft/build/armeabi-v7a 
cmake ../.. -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI=armeabi-v7a -DOBOE_SOURCE=`pwd`/../../../oboe -DANDROID_PLATFORM=16
make -j8
cd -

mkdir -p openal-soft/build/arm64-v8a && cd openal-soft/build/arm64-v8a
cmake ../.. -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI=arm64-v8a -DOBOE_SOURCE=`pwd`/../../../oboe -DANDROID_PLATFORM=16
make -j8
cd -

mkdir -p openal-soft/build/x86 && cd openal-soft/build/x86
cmake ../.. -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI=x86 -DOBOE_SOURCE=`pwd`/../../../oboe -DANDROID_PLATFORM=16
make -j8
cd -

mkdir -p openal-soft/build/x86_64 && cd openal-soft/build/x86_64
cmake ../.. -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI=x86_64 -DOBOE_SOURCE=`pwd`/../../../oboe -DANDROID_PLATFORM=16
make -j8
cd -

mkdir -p lib/arm64-v8a
mkdir -p lib/armeabi-v7a
mkdir -p lib/x86_64
mkdir -p lib/x86
mkdir -p include

cp openal-soft/build/arm64-v8a/libopenal.so lib/arm64-v8a
cp openal-soft/build/armeabi-v7a/libopenal.so lib/armeabi-v7a
cp openal-soft/build/x86_64/libopenal.so lib/x86_64
cp openal-soft/build/x86/libopenal.so lib/x86
cp -r openal-soft/include/AL include/AL
rm -rf openal-soft
rm -rf oboe
