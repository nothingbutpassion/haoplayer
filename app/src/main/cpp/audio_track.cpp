#include <jni.h>
#include "log.h"
#include "audio_track.h"

#undef  LOG_TAG
#define LOG_TAG "AudioTrack"

//
// JNI related
//
extern JNIEnv* getJNIEnv(void);

static void callVoidMethod(jobject obj, const char* name, const char* sig, ...) {
	JNIEnv* env = getJNIEnv();
	jclass cls = env->GetObjectClass(obj);
	if (!cls) {
		LOGE("callVoidMethod: GetObjectClass failed");
		return;
	}
	jmethodID mID = env->GetMethodID(cls, name, sig);
	if (!mID) {
		LOGE("callVoidMethod: GetMethodID: name=%s, sig=%s failed", name, sig);
		return;
	}

    va_list args;
    va_start(args, sig);
    env->CallVoidMethodV(obj, mID, args);
	va_end(args);
}

#define CALL_OBJECT_METHOD_IMPLEMENT(jtype, type)										\
static jtype call##type##Method(jobject obj, const char* name, const char* sig, ...) {			\
	jtype result;																			\
	JNIEnv* env = getJNIEnv();																\
	if (!env) {																				\
		LOGE("call"#type"Method: JNIEnv pointer is null");								\
		return result;																		\
	}																						\
	jclass cls = env->GetObjectClass(obj);													\
	if (!cls) {																				\
		LOGE("call"#type"Method: GetObjectClass failed");								\
		return result;																		\
	}																						\
	jmethodID mID = env->GetMethodID(cls, name, sig);										\
	if (!mID) {																				\
		LOGE("call"#type"Method: GetMethodID: name=%s, sig=%s failed", name, sig);	\
		return result;																		\
	}																						\
    va_list args;																			\
    va_start(args, sig);																	\
    result = env->Call##type##MethodV(obj, mID, args);										\
	va_end(args);																			\
    env->DeleteLocalRef(cls);																\
	return result;																			\
}

CALL_OBJECT_METHOD_IMPLEMENT(jobject, Object)
CALL_OBJECT_METHOD_IMPLEMENT(jboolean, Boolean)
CALL_OBJECT_METHOD_IMPLEMENT(jbyte, Byte)
CALL_OBJECT_METHOD_IMPLEMENT(jchar, Char)
CALL_OBJECT_METHOD_IMPLEMENT(jshort, Short)
CALL_OBJECT_METHOD_IMPLEMENT(jint, Int)
CALL_OBJECT_METHOD_IMPLEMENT(jlong, Long)
CALL_OBJECT_METHOD_IMPLEMENT(jfloat, Float)
CALL_OBJECT_METHOD_IMPLEMENT(jdouble, Double)

jobject newAudioTrack(int sampleRateInHZ)
{
	jobject audioTrackObject = 0;
	JNIEnv* env = getJNIEnv();
	jclass audioTrackClass = env->FindClass("android/media/AudioTrack");
	if (!audioTrackClass) {
		LOGE("newAudioTrack: FindClass: android/media/AudioTrack failed");
		return audioTrackObject;
	}

	jmethodID constructor = env->GetMethodID(audioTrackClass, "<init>", "(IIIIII)V");
	if (!constructor) {
		LOGE("newAudioTrack: GetMethodID: <init>, (IIIIII)V failed");
		return audioTrackObject;
	}

	// refer to android offical doc
	const int CHANNEL_OUT_STEREO = 0xc;
	const int ENCODING_PCM_16BIT = 0x2;
	const int STREAM_MUSIC = 0x3;
	const int MODE_STREAM = 0x1;

	// int getMinBufferSize (int sampleRateInHz, int channelConfig, int audioFormat)
	jmethodID getMinBufferSize = env->GetStaticMethodID(audioTrackClass, "getMinBufferSize", "(III)I");
	const int bufferSize = env->CallStaticIntMethod(audioTrackClass, getMinBufferSize, sampleRateInHZ,
													CHANNEL_OUT_STEREO, ENCODING_PCM_16BIT);
	//
	// AudioTrack(int streamType, int sampleRateInHz, int channelConfig,
	//			  int audioFormat, int bufferSizeInBytes, int mode)
	//
	audioTrackObject = env->NewObject(audioTrackClass, constructor, STREAM_MUSIC, sampleRateInHZ,
									  CHANNEL_OUT_STEREO, ENCODING_PCM_16BIT, bufferSize, MODE_STREAM);
	if (!audioTrackObject) {
		LOGE("newAudioTrack: NewObject failed");
		return audioTrackObject;
	}

	audioTrackObject = env->NewGlobalRef(audioTrackObject);
	if (!audioTrackObject) {
		LOGE("newAudioTrack: NewGlobalRef failed");
		return audioTrackObject;
	}
	return audioTrackObject;
}

void deleteAudioTrack(jobject audioTrack) {
	JNIEnv* env = getJNIEnv();
	env->DeleteGlobalRef(audioTrack);
}


void playAudioTrack(jobject audioTrack) {
    //
    // void play()
    //
	callVoidMethod(audioTrack, "play", "()V");
}

int writeAudioTrack(jobject audioTrack, void* buffer, int len) {
	JNIEnv* env = getJNIEnv();
	jbyteArray array = env->NewByteArray(len);
	env->SetByteArrayRegion(array, 0, len, (const jbyte*)buffer);
	//
	// int write(byte[] buffer, int offset, int size);
	//
	int count = callIntMethod(audioTrack, "write", "([BII)I", array, 0, len);
	env->DeleteLocalRef(array);
	return count;
}

int getAudioTrackPosition(jobject audioTrack) {
    //
    // int getPlaybackHeadPosition();
    //
    return callIntMethod(audioTrack, "getPlaybackHeadPosition", "()I");
}

int getAudioTrackPlayState(jobject audioTrack) {
    //
    // int getPlayState();
    //
    return callIntMethod(audioTrack, "getPlayState", "()I");
}

void stopAudioTrack(jobject audioTrack) {
    //
    // void play()
    //
    callVoidMethod(audioTrack, "stop", "()V");
}

void pauseAudioTrack(jobject audioTrack) {
    //
    // void pause()
    //
    callVoidMethod(audioTrack, "pause", "()V");
}

void flushAudioTrack(jobject audioTrack) {
    //
    // void flush()
    //
    callVoidMethod(audioTrack, "flush", "()V");
}

void releaseAudioTrack(jobject audioTrack) {
    //
    // void release()
    //
    callVoidMethod(audioTrack, "release", "()V");
}