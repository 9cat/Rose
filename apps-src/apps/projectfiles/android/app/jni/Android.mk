LOCAL_PATH := $(call my-dir)/../../../..

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL/SDL2-2.0.4

LOCAL_C_INCLUDES := $(LOCAL_PATH)/external/boost \
	$(LOCAL_PATH)/external/bzip2 \
	$(LOCAL_PATH)/external/zlib \
	$(LOCAL_PATH)/../linker/include/SDL2 \
	$(LOCAL_PATH)/../linker/include/SDL2_image \
	$(LOCAL_PATH)/../linker/include/SDL2_mixer \
	$(LOCAL_PATH)/../linker/include/SDL2_net \
	$(LOCAL_PATH)/../linker/include/SDL2_ttf \
	$(LOCAL_PATH)/librose \
	$(LOCAL_PATH)/studio
	
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/studio/*.c) \
	$(wildcard $(LOCAL_PATH)/studio/*.cpp) \
	$(wildcard $(LOCAL_PATH)/studio/gui/dialogs/*.c) \
	$(wildcard $(LOCAL_PATH)/studio/gui/dialogs/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/event/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/event/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/iterator/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/iterator/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/widget_Definition/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/widget_Definition/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/window_Builder/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/auxiliary/window_Builder/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/dialogs/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/dialogs/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/lib/types/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/lib/types/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/gui/widgets/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/gui/widgets/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/plot/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/plot/*.cpp) \
	$(wildcard $(LOCAL_PATH)/librose/serialization/*.c) \
	$(wildcard $(LOCAL_PATH)/librose/serialization/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/iostreams/src/*.c) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/iostreams/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/regex/src/*.c) \
	$(wildcard $(LOCAL_PATH)/external/boost/libs/regex/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/gettext/gettext-runtime/intl/*.c) \
	$(wildcard $(LOCAL_PATH)/external/gettext/gettext-runtime/intl/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/libiconv/lib/*.c) \
	$(wildcard $(LOCAL_PATH)/external/libiconv/lib/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/bzip2/*.c) \
	$(wildcard $(LOCAL_PATH)/external/bzip2/*.cpp) \
	$(wildcard $(LOCAL_PATH)/external/zlib/*.c) \
	$(wildcard $(LOCAL_PATH)/external/zlib/*.cpp)) 
	
#LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_net SDL2_ttf

LOCAL_STATIC_LIBRARIES := libgnustl_shared.so

#LOCAL_LDLIBS := -llog
LOCAL_LDLIBS := -llog -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_net -lSDL2_ttf

include $(BUILD_SHARED_LIBRARY)
