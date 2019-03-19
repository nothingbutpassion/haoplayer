#pragma once

#include <thread>
#include "element.h"
#include "ffwrapper.h"
#include "utils.h"

class VideoDecoder: public Element {

public:   
    VideoDecoder();
    ~VideoDecoder();

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
    
    int onEvent(const Event& event) override;
    int onBuffer(const Buffer& buffer) override;

public:
    void setEngine(FFWrapper* ffWrapper) {
        this->ffWrapper = ffWrapper;
    }
    void setSource(Element* demuxer) {
        this->demuxer = demuxer;
    }
    void setVideoSink(Element* videoSink) {
        this->videoSink = videoSink;
    }

private:
    int toNull();
    int toReady();
    int toPaused();
    int toPlaying();
    void decoding();

private:
    Clock* clock = nullptr;
    Bus* bus = nullptr;
    FFWrapper* ffWrapper = nullptr;
    Element* demuxer = nullptr;
    Element* videoSink = nullptr;
    States states;
    std::thread decodingThread;
    Queue<Event> eventQueue;

private:
    struct BufferCompare {
        bool operator()(const AVPacket& lPacket, const AVPacket& rPacket);
    };
    PriorityQueue<AVPacket, BufferCompare> bufferQueue;
};