include $(CLEAR_VARS)
LOCAL_PATH 		:= $(ROOT_PATH)/lib/kglt

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/boost/include/boost-1_53
LOCAL_LDLIBS 	+= -L$(ROOT_PATH)/lib/boost/lib/ -lboost_system

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/kazmath
LOCAL_LDLIBS	+= -L -kazmath

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/kaztimer
LOCAL_LDLIBS	+= -L -kaztimer

#Add LUA
LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/lua-5.2.2/src
LOCAL_LDLIBS	+= -L -lua

#OpenAL
LOCAL_CFLAGS	+= -I$(ROOT_PATH)/lib/OpenAL/include
LOCAL_LDLIBS	+= -L -lopenal

LOCAL_CFLAGS 	+= -I$(ROOT_PATH)/lib/SDL2-2.0.0/include
LOCAL_LDLIBS	+= -L -SDL2

LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/kglt/ui/rocket/Include
LOCAL_CFLAGS 	+= -I$(LOCAL_PATH)/submodules/luabind

LOCAL_CFLAGS	+= -I$(ROOT_PATH)/lib/soil
LOCAL_CFLAGS	+= -L -soil

LOCAL_CFLAGS	+= -I$(ROOT_PATH)/lib/ode-0.12/include
LOCAL_CFLAGS	+= -L -ode
LOCAL_CPPFLAGS  += -DdSINGLE

LOCAL_CPPFLAGS 	+= -fexceptions
LOCAL_CPPFLAGS 	+= -frtti
LOCAL_CPPFLAGS  += -std=c++11

LOCAL_MODULE	:= kglt
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/kglt/**/*.cpp))
include $(BUILD_SHARED_LIBRARY) 
