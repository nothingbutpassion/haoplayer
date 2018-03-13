#pragma once

#include <jni.h>

// The return value of getAudioTrackPlayState()
#define PLAYSTATE_STOPPED   (0x00000001)
#define PLAYSTATE_PAUSED    (0x00000002)
#define PLAYSTATE_PLAYING   (0x00000003)

jobject newAudioTrack(int sampleRateInHZ);
void deleteAudioTrack(jobject audioTrack);
void playAudioTrack(jobject audioTrack);
int writeAudioTrack(jobject audioTrack, void* buffer, int len);
int getAudioTrackPosition(jobject audioTrack);
int getAudioTrackPlayState(jobject audioTrack);
void stopAudioTrack(jobject audioTrack);
void pauseAudioTrack(jobject audioTrack);
void flushAudioTrack(jobject audioTrack);
void releaseAudioTrack(jobject audioTrack);




