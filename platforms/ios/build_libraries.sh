#!/bin/bash
#
# Builds libsimulant.a for iOS devices and the iOS simulator, and packages
# them as an XCFramework (which is what Xcode wants), from the directory it's
# run in:
#
#   ios-arm64-clang/<type>/simulant/libsimulant.a      Devices (arm64)
#   ios-simulator-clang/<type>/simulant/libsimulant.a  The simulator (arm64 and x86_64)
#   ios-xcframework/<type>/simulant.xcframework        Both, for Xcode
#
# Usage: build_libraries.sh <Debug|Release>
#
# IOS_DEVICE_SDK and IOS_SIMULATOR_SDK must be the paths of the iPhoneOS and
# iPhoneSimulator SDKs. Runs in the image from platforms/ios/Dockerfile.
#

set -euo pipefail

BUILD_TYPE=${1:?Usage: $0 <Debug|Release>}
TYPE=$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')
SOURCE_DIR=$(cd "$(dirname "$0")/../.." && pwd)
JOBS=${JOBS:-$(nproc)}

: "${IOS_DEVICE_SDK:?Set IOS_DEVICE_SDK to the path of an iPhoneOS SDK}"
: "${IOS_SIMULATOR_SDK:?Set IOS_SIMULATOR_SDK to the path of an iPhoneSimulator SDK}"

build() {
    local name=$1 sdk=$2 arch=$3
    local dir=build-ios/${name}-${TYPE}

    echo "=== Building ${name} (${BUILD_TYPE}) ==="
    cmake -S "${SOURCE_DIR}" -B "${dir}" \
        -DCMAKE_TOOLCHAIN_FILE="${SOURCE_DIR}/toolchains/iOS.cmake" \
        -DIOS_SDK="${sdk}" \
        -DIOS_ARCH="${arch}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DSIMULANT_BUILD_SAMPLES=OFF \
        -DSIMULANT_BUILD_TESTS=OFF
    cmake --build "${dir}" --target simulant -j "${JOBS}"
}

build device-arm64 "${IOS_DEVICE_SDK}" arm64
build simulator-arm64 "${IOS_SIMULATOR_SDK}" arm64
build simulator-x86_64 "${IOS_SIMULATOR_SDK}" x86_64

DEVICE_LIB=ios-arm64-clang/${TYPE}/simulant/libsimulant.a
SIMULATOR_LIB=ios-simulator-clang/${TYPE}/simulant/libsimulant.a

mkdir -p "$(dirname "${DEVICE_LIB}")" "$(dirname "${SIMULATOR_LIB}")"
cp "build-ios/device-arm64-${TYPE}/simulant/libsimulant.a" "${DEVICE_LIB}"
lipo -create \
    "build-ios/simulator-arm64-${TYPE}/simulant/libsimulant.a" \
    "build-ios/simulator-x86_64-${TYPE}/simulant/libsimulant.a" \
    -output "${SIMULATOR_LIB}"

# The XCFramework, put together by hand as xcodebuild only runs on macOS
XCFRAMEWORK=ios-xcframework/${TYPE}/simulant.xcframework
rm -rf "${XCFRAMEWORK}"

slice() {
    local id=$1 lib=$2
    mkdir -p "${XCFRAMEWORK}/${id}/Headers"
    cp "${lib}" "${XCFRAMEWORK}/${id}/libsimulant.a"

    # So that apps can #include <simulant/simulant.h>
    rsync -am -L \
        --include='*.h' --include='*.hpp' --include='*.inc' --include='*.inl' \
        -f 'hide,! */' "${SOURCE_DIR}/simulant" "${XCFRAMEWORK}/${id}/Headers/"
}

slice ios-arm64 "${DEVICE_LIB}"
slice ios-arm64_x86_64-simulator "${SIMULATOR_LIB}"

cat > "${XCFRAMEWORK}/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>AvailableLibraries</key>
	<array>
		<dict>
			<key>BinaryPath</key>
			<string>libsimulant.a</string>
			<key>HeadersPath</key>
			<string>Headers</string>
			<key>LibraryIdentifier</key>
			<string>ios-arm64</string>
			<key>LibraryPath</key>
			<string>libsimulant.a</string>
			<key>SupportedArchitectures</key>
			<array>
				<string>arm64</string>
			</array>
			<key>SupportedPlatform</key>
			<string>ios</string>
		</dict>
		<dict>
			<key>BinaryPath</key>
			<string>libsimulant.a</string>
			<key>HeadersPath</key>
			<string>Headers</string>
			<key>LibraryIdentifier</key>
			<string>ios-arm64_x86_64-simulator</string>
			<key>LibraryPath</key>
			<string>libsimulant.a</string>
			<key>SupportedArchitectures</key>
			<array>
				<string>arm64</string>
				<string>x86_64</string>
			</array>
			<key>SupportedPlatform</key>
			<string>ios</string>
			<key>SupportedPlatformVariant</key>
			<string>simulator</string>
		</dict>
	</array>
	<key>CFBundlePackageType</key>
	<string>XFWK</string>
	<key>XCFrameworkFormatVersion</key>
	<string>1.0</string>
</dict>
</plist>
EOF

echo "=== Built ==="
lipo -info "${DEVICE_LIB}" "${SIMULATOR_LIB}"
