#pragma once

#include <vector>
#include <jni.h>
#include "ffwrapper.h"
#include "demuxer.h"
#include "audio_decoder.h"
#include "audio_render.h"
#include "video_decoder.h"
#include "video_render.h"
#include "player.h"

struct Player {
    static Player& instance() {
        static Player player;
        return player;
    }
    void setDataSource(const char* url);
    void setSurface(jobject surface);
    void play();
    void stop();
    void pause();
    void seek(int position);
    int getDuration();
    int getPosition();

private:
    Player();
    ~Player();

private:
    bool validStates();

private:
    FFWrapper ffWrapper;
    Demuxer demuxer;
    VideoDecoder videoDecoder;
    VideoRender videoRender;
    AudioDecoder audioDecoder;
    AudioRender audioRender;
    Clock* clock = nullptr;
    Bus* bus = nullptr;
    std::vector<Element*> elememts{&demuxer, &videoDecoder, &audioDecoder, &videoRender, &audioRender};
};
