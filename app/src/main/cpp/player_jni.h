#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_hao_player_Player_nativeInit(JNIEnv*, jclass);
JNIEXPORT void JNICALL Java_com_hao_player_Player_setSurface(JNIEnv*, jclass, jobject);
JNIEXPORT void JNICALL Java_com_hao_player_Player_setDataSource(JNIEnv*, jclass, jstring);
JNIEXPORT void JNICALL Java_com_hao_player_Player_play(JNIEnv*, jclass);
JNIEXPORT void JNICALL Java_com_hao_player_Player_pause(JNIEnv*, jclass);
JNIEXPORT void JNICALL Java_com_hao_player_Player_stop(JNIEnv*, jclass);
JNIEXPORT void JNICALL Java_com_hao_player_Player_seek(JNIEnv*, jclass, jint);
JNIEXPORT jint JNICALL Java_com_hao_player_Player_getDuration(JNIEnv*, jclass);

#ifdef __cplusplus
}
#endif

