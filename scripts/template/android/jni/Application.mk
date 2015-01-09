APP_BUILD_SCRIPT        := jni/Android.mk
APP_PROJECT_PATH        := {PROJECT_OUTPUT_DEST}
NDK_TOOLCHAIN_VERSION   := clang
APP_CFLAGS              := -I$(APP_PROJECT_PATH)/jni/lib/pcre \
    -I$(APP_PROJECT_PATH)/jni/lib/kazmath \
    -I$(APP_PROJECT_PATH)/jni/lib/kaztimer \
    -I$(APP_PROJECT_PATH)/jni/lib/kazbase \
    -I$(APP_PROJECT_PATH)/jni/include/sdl \
    -I$(APP_PROJECT_PATH)/jni/lib/sdl/src \
    -I$(APP_PROJECT_PATH)/jni/lib/soil/src \
    -I$(APP_PROJECT_PATH)/jni/lib/openal/OpenAL/include \
    -I$(APP_PROJECT_PATH)/jni/lib/freetype2/include \
    -I$(APP_PROJECT_PATH)/jni/lib/ode/include \
    -I$(APP_PROJECT_PATH)/jni/lib/lua/src \
    -I$(APP_PROJECT_PATH)/jni/lib/tinyxml \
    -I$(APP_PROJECT_PATH)/jni/lib/kglt \
    -I$(APP_PROJECT_PATH)/jni/gpg_sdk/include

APP_CPPFLAGS            += "-std=c++11" -frtti -fexceptions
APP_STL                 := c++_shared
APP_PLATFORM            := android-19
APP_ABI                 := armeabi-v7a x86
APP_OPTIM               := debug
