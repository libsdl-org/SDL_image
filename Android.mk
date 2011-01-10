LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SDL_image

LOCAL_CFLAGS := -I$(LOCAL_PATH)/../jpeg -I$(LOCAL_PATH)/../png -I$(LOCAL_PATH)/../SDL/include \
				-DLOAD_PNG -DLOAD_JPG -DLOAD_GIF -DLOAD_BMP

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_SHARED_LIBRARIES := SDL

LOCAL_STATIC_LIBRARIES := png jpeg

LOCAL_LDLIBS := -lz

include $(BUILD_SHARED_LIBRARY)
