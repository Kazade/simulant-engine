LOCAL_PATH 		:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := kglt-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libkglt.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := kazmath-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libkazmath.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := kazbase-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libkazbase.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := kaztimer-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libkaztimer.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := lua-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/liblua.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := pcre-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libpcre.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := soil-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libsoil.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := openal-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libopenal.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype2-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libfreetype2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sdl-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libSDL2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tinyxml-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libtinyxml.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ode-prebuilt
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libode.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := google-play-games
LOCAL_SRC_FILES := gpg_sdk/lib/c++/$(TARGET_ARCH_ABI)/libgpg.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)

define all-cpp-files-under
$(patsubst ./%, %, \
  $(shell cd $(LOCAL_PATH) ; \
          find -L $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_MODULE    := main
LOCAL_SRC_FILES := $(call all-cpp-files-under, {PROJECT_NAME}) ./SDL_android_main.c
LOCAL_LDLIBS := -landroid -llog
LOCAL_LDLIBS += -lGLESv2 -lz
LOCAL_CFLAGS := -I$(LOCAL_PATH)/include \
    -I$(LOCAL_PATH)/include/sdl \
    -I$(LOCAL_PATH)/include/ode \
    -I$(LOCAL_PATH)/include/sdl/src \
    -I$(LOCAL_PATH)/include/kazbase \
    -I$(LOCAL_PATH)/gpg_sdk/include

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := \
    kazmath-prebuilt \
    kazbase-prebuilt \
    kaztimer-prebuilt \
    lua-prebuilt \
    pcre-prebuilt \
    soil-prebuilt \
    openal-prebuilt \
    freetype2-prebuilt \
    sdl-prebuilt \
    tinyxml-prebuilt \
    ode-prebuilt \
    kglt-prebuilt \
    google-play-games

LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_CPPFLAGS 	+= -frtti
LOCAL_CPPFLAGS  += -std=c++11 -g

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
