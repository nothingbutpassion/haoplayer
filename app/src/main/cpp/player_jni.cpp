#include <pthread.h>
#include "log.h"
#include "player_jni.h"
#include "player.h"

#undef  LOG_TAG
#define LOG_TAG  "player_jni"

static pthread_key_t gThreadKey;
static JavaVM* gJavaVM;
static jobject gSurface;

JNIEnv* getJNIEnv(void) {
    JNIEnv* env = nullptr;
    int err = gJavaVM->AttachCurrentThread(&env, 0);
    if(err < 0) {
        LOGE("Failed to attach current thread");
        return 0;
    }
    if (!pthread_getspecific(gThreadKey)) {
        pthread_setspecific(gThreadKey, env);
    }
    return env;
}

void setupThread(void) {
    pthread_setspecific(gThreadKey, (void*)getJNIEnv());
}

static void destoryThread(void* value) {
    JNIEnv* env = static_cast<JNIEnv*>(value);
    if (env) {
        gJavaVM->DetachCurrentThread();
        pthread_setspecific(gThreadKey, 0);
    }
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad");
    gJavaVM = vm;
    JNIEnv *env;
    if (gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get the environment using GetEnv()");
        return -1;
    }
    if (pthread_key_create(&gThreadKey, destoryThread)) {
        LOGE("Error initializing pthread key");
    } else {
        //setupThread();
    }
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_nativeInit(JNIEnv*, jclass) {
    // TODO: save methodID, fieldID, ...
    LOGI("Java_com_hao_player_Player_nativeInit Enter");
    LOGI("Java_com_hao_player_Player_nativeInit Exit");
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_setSurface(JNIEnv* env, jclass, jobject surface) {
    LOGI("Java_com_hao_player_Player_setSurface Enter");
    gSurface = env->NewGlobalRef(surface);
    Player::instance().setSurface(gSurface);
    LOGI("Java_com_hao_player_Player_setSurface Exit");
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_setDataSource(JNIEnv* env, jclass, jstring source) {
    LOGI("Java_com_hao_player_Player_setDataSource Enter");
    const char* url = env->GetStringUTFChars(source, 0);
    Player::instance().setDataSource(url);
    env->ReleaseStringUTFChars(source, url);
    LOGI("Java_com_hao_player_Player_setDataSource Exit");
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_play(JNIEnv*, jclass) {
    LOGI("Java_com_hao_player_Player_play Enter");
    Player::instance().play();
    LOGI("Java_com_hao_player_Player_play Exit");
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_pause(JNIEnv*, jclass) {
    LOGI("Java_com_hao_player_Player_pause Enter");
    Player::instance().pause();
    LOGI("Java_com_hao_player_Player_pause Exit");
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_stop(JNIEnv* env, jclass)
{
    LOGI("Java_com_hao_player_Player_stop Enter");
    Player::instance().stop();
    env->DeleteGlobalRef(gSurface);
    LOGI("Java_com_hao_player_Player_stop Exit");
}

JNIEXPORT void JNICALL Java_com_hao_player_Player_seek(JNIEnv*, jclass, jint position)
{
    LOGI("Java_com_hao_player_Player_seek Enter");
    Player::instance().seek(position);
    LOGI("Java_com_hao_player_Player_seek Exit");
}

JNIEXPORT jint JNICALL Java_com_hao_player_Player_getDuration(JNIEnv*, jclass)
{
    LOGI("Java_com_hao_player_Player_getDuration Enter");
    Player::instance().getDuration();
    LOGI("Java_com_hao_player_Player_getDuration Exit");
}

