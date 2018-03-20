#pragma once

#include <string>
#include <thread>
#include "element.h"
#include "ffwrapper.h"
#include "utils.h"

class Demuxer: public Element {

public:   
    Demuxer();
    ~Demuxer();

    void setBus(Bus* bus) override {
        this->bus = bus;
    }
    
    void setClock(Clock* clock) override {
        this->clock = clock;
    }
    Clock* getClock() override {
        return clock;
    }

    int setState(State state) override;
    State getState() override {
        return states.getCurrent();
    }
    
    int sendEvent(const Event& event) override;
    int pushBuffer(const Buffer& buffer) override;

public:
    void setEngine(FFWrapper* ffWrapper) {
        this->ffWrapper = ffWrapper;
    }
    void setSource(const std::string url) {
        this->url = url;
    }
    void setAudioSink(Element* audioSink) {
        this->audioSink = audioSink;
    }
    void setVideoSink(Element* videoSink) {
        this->videoSink = videoSink;
    }

public:
    int getDuration() {
        return ffWrapper->duration()*1000/AV_TIME_BASE;
    }
    void seek(int position) {
        ffWrapper->seek(int64_t(position)*AV_TIME_BASE/1000);
    }


private:
    int toIdle();
    int toReady();
    int toPaused();
    int toPlaying();

    
private:
    void demuxing();

private:

    Clock* clock = nullptr;
    Bus* bus = nullptr;
    FFWrapper* ffWrapper = nullptr;
    Element* audioSink = nullptr;
    Element* videoSink = nullptr;
    std::string url;
    States states;
    std::thread demuxingThread;
    Queue<Event> eventQueue;

};