LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

PARENT_PATH = $(LOCAL_PATH)/..

LOCAL_CFLAGS 	+= -I$(PARENT_PATH)/kazmath
LOCAL_SHARED_LIBRARIES := kazmath

LOCAL_CFLAGS 	+= -I$(PARENT_PATH)/kaztimer
LOCAL_SHARED_LIBRARIES += kaztimer

LOCAL_CFLAGS 	+= -I$(PARENT_PATH)/kazbase
LOCAL_SHARED_LIBRARIES += kazbase

#Add LUA
#LOCAL_CFLAGS 	+= -I$(PARENT_PATH)/lua-5.2.2/src
#LOCAL_SHARED_LIBRARIES += lua

#OpenAL
LOCAL_CFLAGS	+= -I$(PARENT_PATH)/openal/OpenAL/include
LOCAL_SHARED_LIBRARIES += openal

LOCAL_CFLAGS	+= -I$(PARENT_PATH)/freetype2/include
LOCAL_SHARED_LIBRARIES += freetype

LOCAL_CFLAGS 	+= -I$(PARENT_PATH)/sdl/include
LOCAL_SHARED_LIBRARIES += SDL2

LOCAL_CFLAGS	+= -I$(PARENT_PATH)/soil
LOCAL_SHARED_LIBRARIES += soil

LOCAL_CFLAGS	+= -I$(PARENT_PATH)/ode/ode/include
LOCAL_SHARED_LIBRARIES += ode

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kglt/ui/rocket/Include
LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/submodules/luabind

LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_CPPFLAGS 	+= -frtti
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_CPPFLAGS  += -DdSINGLE #For ODE

LOCAL_MODULE	:= kglt

define all-cpp-files-under
$(patsubst ./%, %, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_SRC_FILES := $(call all-cpp-files-under, kglt)
LOCAL_LDLIBS    += -lGLESv3

include $(BUILD_SHARED_LIBRARY)
