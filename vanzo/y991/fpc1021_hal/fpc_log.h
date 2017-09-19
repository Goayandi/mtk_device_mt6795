/**
 * Copyright 2011 Fingerprint Cards AB
 */
#ifndef FPC1080_LOG_H
#define FPC1080_LOG_H

#ifndef NO_ANDROID_LOG
#include <android/log.h>
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif// NO_ANDROID_LOG

#ifdef NO_ANDROID_LOG
#define LOGE(...) {}
#define LOGW(...) {}
#define LOGI(...) {}
#define LOGD(...) {}
#endif // NO_ANDROID_LOG

#endif // FPC1080_LOG_H

