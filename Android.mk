LOCAL_PATH := $(call my-dir)
common_cflags := -Wall -Werror
#
# Executable
#

include $(CLEAR_VARS)
LOCAL_MODULE := firewalld
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(common_cflags)
LOCAL_SRC_FILES := firewalld.c
LOCAL_SHARED_LIBRARIES := libcutils liblog
include $(BUILD_EXECUTABLE)
