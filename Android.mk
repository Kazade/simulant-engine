LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#OpenAL
LOCAL_CFLAGS	+= -I$(LOCAL_PATH)/openal/OpenAL/include
LOCAL_SHARED_LIBRARIES += libopenal

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/sdl/include
LOCAL_SHARED_LIBRARIES += libSDL2

LOCAL_CFLAGS    += -I$(LOCAL_PATH)/../simulant/simulant/deps
LOCAL_SHARED_LIBRARIES += libtinyxml

LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_CPPFLAGS 	+= -frtti
LOCAL_CPPFLAGS  += -std=c++11 -g

LOCAL_MODULE	:= simulant

define all-cpp-files-under
$(patsubst ./%, %, \
  $(shell cd $(LOCAL_PATH) ; \
          find -L $(1) -name "*.cpp" -and -not -name ".*" -and -not -name "kos_*.cpp") \
 )
endef

LOCAL_SRC_FILES := $(call all-cpp-files-under, simulant)
LOCAL_SRC_FILES += simulant/loaders/stb_vorbis.c
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
LOCAL_LDLIBS    += -lGLESv1_CM -lGLESv2 -lz -ldl -lEGL -llog -landroid
LOCAL_EXPORT_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -lEGL -llog -landroid -lz

include $(BUILD_SHARED_LIBRARY)
