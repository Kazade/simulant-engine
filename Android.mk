LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kazmath
LOCAL_SHARED_LIBRARIES := libkazmath

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kaztimer
LOCAL_SHARED_LIBRARIES += libkaztimer

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kazbase
LOCAL_SHARED_LIBRARIES += libkazbase

#Add LUA
#LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/lua-5.2.2/src
#LOCAL_SHARED_LIBRARIES += lua

#OpenAL
LOCAL_CFLAGS	+= -I$(LOCAL_PATH)/openal/OpenAL/include
LOCAL_SHARED_LIBRARIES += libopenal

LOCAL_CFLAGS	+= -I$(LOCAL_PATH)/freetype2/include
LOCAL_SHARED_LIBRARIES += libfreetype2

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/sdl/include
LOCAL_SHARED_LIBRARIES += libSDL2

LOCAL_CFLAGS	+= -I$(LOCAL_PATH)/soil
LOCAL_SHARED_LIBRARIES += libsoil

LOCAL_CFLAGS	+= -I$(LOCAL_PATH)/ode/ode/include
LOCAL_SHARED_LIBRARIES += libode

LOCAL_SHARED_LIBRARIES += libtinyxml liblua

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kglt/ui/rocket/Include
LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/submodules/luabind

LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_CPPFLAGS 	+= -frtti
LOCAL_CPPFLAGS  += -std=c++11 -g
LOCAL_CPPFLAGS  += -DdIDESINGLE #For ODE

LOCAL_MODULE	:= kglt

define all-cpp-files-under
$(patsubst ./%, %, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_SRC_FILES := $(call all-cpp-files-under, kglt)
LOCAL_SRC_FILES += $(call all-cpp-files-under, submodules/luabind/src)
LOCAL_SRC_FILES += kglt/loaders/stb_vorbis.c
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
LOCAL_LDLIBS    += -lGLESv1_CM -lGLESv2 -lz -ldl -lEGL -llog -landroid
LOCAL_EXPORT_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -lEGL -llog -landroid -lz

include $(BUILD_SHARED_LIBRARY)
