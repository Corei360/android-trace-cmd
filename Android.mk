#Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
BUILD_TCMD := true

ifeq ($(BUILD_TCMD),true)
LOCAL_PATH := $(call my-dir)
COMMON_CFLAGS := -DWARN_NO_AUDIT -DNO_AUDIT
COMMON_LIBS := parsevent tracecmd

# Build parevent lib
include $(CLEAR_VARS)

LOCAL_SRC_FILES := event-parse.c trace-seq.c parse-filter.c \
		trace-read.c parse-utils.c str_error_r.c

LOCAL_C_INCLUDES := $(common_target_c_includes) \
		$(LOCAL_PATH)/include
LOCAL_CFLAGS := $(COMMON_CFLAGS) $(common_target_cflags)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := parsevent

include $(BUILD_STATIC_LIBRARY)



# Build trace-cmd lib
include $(CLEAR_VARS)

LOCAL_SRC_FILES := trace-util.c trace-input.c trace-ftrace.c \
		trace-output.c glob.c trace-recorder.c trace-usage.c \
		trace-blk-hack.c kbuffer-parse.c event-plugin.c \
		trace-hooks.c trace-msg.c

LOCAL_C_INCLUDES := $(common_target_c_includes) \
		$(LOCAL_PATH)/include
LOCAL_CFLAGS := $(COMMON_CFLAGS) $(common_target_cflags)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := tracecmd 

include $(BUILD_STATIC_LIBRARY)

#
# Build trace-cmd on target
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := trace-cmd.c trace-record.c trace-read.c trace-split.c trace-listen.c \
		trace-stack.c trace-hist.c trace-mem.c trace-snapshot.c trace-stat.c \
		trace-hash.c trace-profile.c trace-stream.c trace-restore.c \
		trace-check-events.c trace-show.c trace-list.c

LOCAL_STATIC_LIBRARIES := $(COMMON_LIBS)
LOCAL_SHARED_LIBRARIES := libcutils libdl
LOCAL_C_INCLUDES := $(common_target_c_includes) \
		$(LOCAL_PATH)/include
LOCAL_CFLAGS := $(COMMON_CFLAGS) $(common_target_cflags)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := trace-cmd

include $(BUILD_EXECUTABLE)

endif
