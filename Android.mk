include $(CLEAR_VARS)
ODE_PATH 		:= $(ROOT_PATH)/lib/ode-0.12
LOCAL_PATH 		:= $(ODE_PATH)
LOCAL_MODULE    := libode
LOCAL_SRC_FILES := ode/src/.libs/libode.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH 		:= $(ROOT_PATH)/lib/kglt

LOCAL_STATIC_LIBRARIES := libode

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/kazmath
LOCAL_SHARED_LIBRARIES := kazmath

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/kaztimer
LOCAL_SHARED_LIBRARIES += kaztimer

#Add LUA
LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/lua-5.2.2/src
LOCAL_SHARED_LIBRARIES += lua

#OpenAL
LOCAL_CFLAGS	+= -I$(ROOT_PATH)/lib/OpenAL/include
LOCAL_SHARED_LIBRARIES += openal

LOCAL_CFLAGS	+= -I$(ROOT_PATH)/lib/freetype-2.4.12/include
LOCAL_LDLIBS 	+= -L$(ROOT_PATH)/lib/freetype-2.4.12/objs/.libs
LOCAL_SHARED_LIBRARIES += freetype

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/SDL2-2.0.0/include
LOCAL_SHARED_LIBRARIES += SDL2

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kglt/ui/rocket/Include
LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/submodules/luabind

LOCAL_CFLAGS	+= -I$(ROOT_PATH)/lib/soil
LOCAL_SHARED_LIBRARIES += soil

LOCAL_C_INCLUDES := $(ROOT_PATH)/lib/ode-0.12/include $(ROOT_PATH)/lib/boost/include/boost-1_53
LOCAL_CPPFLAGS  += -DdSINGLE

LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_CPPFLAGS 	+= -frtti
LOCAL_CPPFLAGS  += -std=c++11

LOCAL_MODULE	:= kglt

define all-cpp-files-under
$(patsubst ./%, %, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_SRC_FILES := $(call all-cpp-files-under, kglt)
LOCAL_LDLIBS    += -lGLESv3

$(warning $(LOCAL_SRC_FILES))

include $(BUILD_SHARED_LIBRARY)
