# The ARMv7 is significanly faster due to the use of the hardware FPU
# APP_ABI := armeabi armeabi-v7a arm64-v8a
APP_ABI := armeabi armeabi-v7a
APP_PLATFORM := android-15

APP_STL := gnustl_shared
APP_GNUSTL_FORCE_CPP_FEATURES := exceptions rtti
APP_CFLAGS += -Wno-error=format-security