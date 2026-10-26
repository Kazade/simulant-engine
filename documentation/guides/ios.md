# iOS Development Guide

This guide covers building Simulant for iOS (iPhone and iPad) and using it in an Xcode project.

> **Status:** iOS support is new. The library is cross-compiled on Linux. The test suite and samples aren't built for iOS yet, and the port hasn't been tried on many devices.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Getting the SDK](#2-getting-the-sdk)
3. [Building the Library](#3-building-the-library)
4. [Using Simulant in an Xcode Project](#4-using-simulant-in-an-xcode-project)
5. [Rendering](#5-rendering)
6. [Audio](#6-audio)
7. [Input and Haptics](#7-input-and-haptics)
8. [App Lifecycle](#8-app-lifecycle)
9. [Files](#9-files)
10. [Limitations](#10-limitations)

---

## 1. Overview

| | |
|---|---|
| **Minimum iOS version** | 14.0 |
| **Architectures** | arm64 (devices), arm64 and x86_64 (simulator) |
| **Renderer** | `gl2x` on OpenGL ES 2.0, through EAGL |
| **Window** | UIKit (`IOSWindow`), not SDL2 |
| **Audio** | ALdc, mixed in software and played through CoreAudio |
| **Library** | Static: `libsimulant.a`, or `simulant.xcframework` |

Simulant is built on Linux with upstream clang and Apple's linker tools ([cctools-port](https://github.com/tpoechtrager/cctools-port)), so you don't need a Mac to build the library. You do need Xcode, and so a Mac, to build, sign and submit an app that uses it.

---

## 2. Getting the SDK

The iOS SDKs (`iPhoneOS.sdk` for devices, `iPhoneSimulator.sdk` for the simulator) come with Xcode. **Apple's license doesn't allow redistributing them**, so they're not in the Docker image and must never be added to a public one. Package them yourself from an Xcode you've downloaded:

```bash
# On a Mac with Xcode installed
cd /Applications/Xcode.app/Contents/Developer/Platforms
VERSION=$(xcrun --sdk iphoneos --show-sdk-version)

tar -C iPhoneOS.platform/Developer/SDKs -cJf ~/iPhoneOS${VERSION}.sdk.tar.xz iPhoneOS.sdk \
    --transform "s/^iPhoneOS.sdk/iPhoneOS${VERSION}.sdk/"
tar -C iPhoneSimulator.platform/Developer/SDKs -cJf ~/iPhoneSimulator${VERSION}.sdk.tar.xz iPhoneSimulator.sdk \
    --transform "s/^iPhoneSimulator.sdk/iPhoneSimulator${VERSION}.sdk/"
```

(`--transform` needs GNU tar, `gtar` from Homebrew. Or copy the `.sdk` directories somewhere, rename them, and archive them from there.) Each archive should contain one directory, e.g. `iPhoneOS26.0.sdk/`.

You can also unpack the SDKs from an Xcode `.xip` on Linux, using [xar](https://github.com/tpoechtrager/xar) and [pbzx](https://github.com/tpoechtrager/pbzx). The [cctools-port](https://github.com/tpoechtrager/cctools-port) and Godot documentation describe how.

### For CI

The GitLab CI jobs download the SDKs when they run. Upload the two archives somewhere private (e.g. a private bucket, behind a signed URL), and add these as **masked** CI/CD variables:

| Variable | Value |
|---|---|
| `IOS_SDK_URL` | URL of the `iPhoneOS*.sdk.tar.xz` archive |
| `IOS_SIMULATOR_SDK_URL` | URL of the `iPhoneSimulator*.sdk.tar.xz` archive |

Without them, the iOS jobs don't run. If they're also *protected*, the jobs only run on protected branches.

---

## 3. Building the Library

The toolchain is in `platforms/ios/Dockerfile`. CI publishes it as the `ios-toolchain` image in the project's registry. To build it yourself:

```bash
docker build -t simulant-ios-toolchain platforms/ios
```

Then, with the SDKs unpacked into `~/ios-sdks`:

```bash
docker run --rm -v $(pwd):/src -v ~/ios-sdks:/sdks -w /src \
    -e IOS_DEVICE_SDK=/sdks/iPhoneOS26.0.sdk \
    -e IOS_SIMULATOR_SDK=/sdks/iPhoneSimulator26.0.sdk \
    simulant-ios-toolchain platforms/ios/build_libraries.sh Release
```

This builds all three slices and leaves:

| Path | Contents |
|---|---|
| `ios-arm64-clang/release/simulant/libsimulant.a` | Devices (arm64) |
| `ios-simulator-clang/release/simulant/libsimulant.a` | The simulator (arm64 and x86_64) |
| `ios-xcframework/release/simulant.xcframework` | Both, with the headers, for Xcode |

To build a single slice, use the CMake toolchain file directly:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../../toolchains/iOS.cmake \
      -DIOS_SDK=/sdks/iPhoneOS26.0.sdk \
      -DIOS_ARCH=arm64 \
      -DCMAKE_BUILD_TYPE=Release ../..
make simulant
```

`toolchains/iOS.cmake` also takes `IOS_DEPLOYMENT_TARGET` (default `14.0`) and `IOS_SIMULATOR` (by default, whether `IOS_SDK` is a simulator SDK).

The CI publishes `ios-arm64-clang.zip`, `ios-simulator-clang.zip` and `ios-xcframework.zip` alongside the other platforms' libraries.

> The library is built without LTO. LTO objects are LLVM bitcode, and Xcode's LLVM is older than the clang the library's built with, so it couldn't read them.

---

## 4. Using Simulant in an Xcode Project

### Linking

1. Add `simulant.xcframework` to the app target (**General → Frameworks, Libraries, and Embedded Content**, "Do Not Embed": it's a static library).
2. Link these system frameworks too: `UIKit`, `QuartzCore`, `OpenGLES`, `AudioToolbox`, `AVFoundation`, `CoreHaptics` and `Foundation`.
3. Simulant is C++20. Set **C++ Language Dialect** to C++20 (or later) and use libc++.

Headers are included as normal, e.g. `#include <simulant/simulant.h>`.

### main()

UIKit has to own the main thread from the start, so your `main()` hands over to `smlt::ios_main()`. It calls your function once the app has a window, and Simulant's main loop runs from there:

```cpp
#include <simulant/simulant.h>

using namespace smlt;

class MyGame: public Application {
public:
    MyGame(const AppConfig& config):
        Application(config) {}

    bool init() override {
        scenes->register_scene<GameScene>("main");
        return true;
    }
};

static int game_main(int argc, char* argv[]) {
    AppConfig config;
    config.title = "My Game";
    config.fullscreen = true;
    config.search_paths.push_back("assets");

    MyGame app(config);
    return app.run();
}

int main(int argc, char* argv[]) {
#ifdef __IOS__
    return smlt::ios_main(argc, argv, game_main);
#else
    return game_main(argc, argv);
#endif
}
```

`ios_main()` never returns. When `game_main()` does, the process exits.

`__IOS__` is defined by `simulant/macros.h` (included by `simulant/simulant.h`) when building for iOS or the iOS simulator.

### Info.plist

`platforms/ios/Info.plist.in` is a starting point. The parts Simulant needs are:

- **`UIApplicationSceneManifest`**: the window is created for a UIKit scene. No scene configurations need listing, as `ios_main()` supplies its own delegate. Apple will require scenes for apps built with SDKs after iOS 26, so there's no non-scene path.
- **`UILaunchScreen`** (an empty dictionary is fine) or a launch storyboard. Without one, iOS runs the app in a letterboxed compatibility mode.
- **`UISupportedInterfaceOrientations`**: the window follows the device to whichever of these orientations it's in, and changes size to match.

### Assets

Add your `assets` directory, and Simulant's own assets (the `simulant` directory from `assets.zip`), to the app target as **folder references** (blue folders), so they're copied into the app bundle with their directory structure kept.

At startup, `ios_main()` changes the working directory to the app bundle, so relative search paths like `"assets"` find them.

---

## 5. Rendering

iOS still supports OpenGL ES, although Apple has deprecated it since iOS 12. Simulant uses the same `gl2x` renderer in ES mode as on Android, through an `EAGLContext`.

- iOS has no default framebuffer. The window creates one (RGBA8 colour, 24-bit depth and 8-bit stencil) at the screen's native resolution, and leaves it bound.
- `window->width()` and `window->height()` are in **pixels**, not points.
- When the device rotates, the framebuffer is recreated at the new size before the next frame.
- `gl1x` isn't available. Requesting it falls back to `gl2x`.

If Apple ever removes OpenGL ES, [ANGLE](https://chromium.googlesource.com/angle/angle) (OpenGL ES on Metal, BSD-licensed) is the planned replacement.

---

## 6. Audio

Sound uses ALdc (Simulant's OpenAL implementation) with its software mixer, played through CoreAudio's RemoteIO unit. Apple's OpenAL framework isn't used.

- Sound pauses while the app's in the background, and resumes when it comes back.
- Sound pauses when the system interrupts it (e.g. a phone call), and resumes when the interruption ends.
- The audio session's category is left at the default (`AVAudioSessionCategorySoloAmbient`), which is silenced by the mute switch and stops other apps' audio. To mix with the player's own music, set the category yourself (`AVAudioSessionCategoryAmbient`) before `ios_main()`.

`SIMULANT_SOUND_DRIVER=null` disables sound, as on other platforms.

---

## 7. Input and Haptics

### Touch

Touches are reported as fingers, as on Android. Each finger gets the lowest free ID, so the first finger down is always `0`. The first finger is also reported as a mouse, for UI that expects one.

### Keyboards

Hardware keyboards work (via `UIKey`, so iOS 13.4 and later). There's no on-screen keyboard. The Escape key doesn't quit the app: apps that quit themselves are rejected from the App Store.

### Game Controllers

Not supported yet.

### Haptics

`window->start_haptics()` plays on the Taptic Engine, through Core Haptics, at the given strength and for the given duration (at most 30 seconds). `has_haptics()` is false on devices without a Taptic Engine, including iPads and the simulator.

---

## 8. App Lifecycle

Simulant's main loop runs on the main thread, and runs UIKit's event loop once a frame (in `IOSWindow::check_events()`).

| Event | What Simulant does |
|---|---|
| App becomes inactive (e.g. Control Centre is opened) | Focus is lost (`signal_focus_lost`), and every finger is lifted |
| App goes into the background | Rendering stops, as iOS kills apps that use the GPU in the background. Sound pauses |
| App returns to the foreground | Rendering and sound resume |

iOS can end a backgrounded app at any time without warning, so save anything important when focus is lost.

---

## 9. Files

- The app bundle (where assets are) is read-only, and is the working directory.
- Temporary files go in the app's own temporary directory (`kfs::temp_dir()`), as `/tmp` isn't writable in the sandbox.
- For anything that should last, use the app's `Documents` or `Library` directory.

---

## 10. Limitations

- The test suite and samples aren't built for iOS yet. They need to be built as signed app bundles to run.
- No game controller support yet.
- No on-screen keyboard.
- Only one window (scene). Extra scenes, e.g. on an iPad, are ignored.
- The library is static only. There's no dynamic framework.
