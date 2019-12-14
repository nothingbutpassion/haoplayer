#pragma once

#define ENABLE_LOG

#ifdef ENABLE_LOG
#include <android/log.h>
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "haoplayer", LOG_TAG " " __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  "haoplayer", LOG_TAG " " __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  "haoplayer", LOG_TAG " " __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "haoplayer", LOG_TAG " " __VA_ARGS__))
#else
#include <android/log.h>
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#endif

