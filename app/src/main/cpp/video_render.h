#pragma once

#include "element.h"
#include "video_device.h"
#include "ffwrapper.h"
#include "utils.h"

class VideoRender: public Element {

public:   
    VideoRender();
    ~VideoRender();

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
        videoDevice->setProperty(VIDEO_ENGIN, ffWrapper);
    }
    void setSource(Element* videoDecoder) {
        this->videoDecoder = videoDecoder;
    }
    void setSurface(void* surface) {
        videoDevice->setProperty(VIDEO_SURFACE, surface);
    }

private:
    int toNull();
    int toReady();
    int toPaused();
    int toPlaying();
    void rendering();

private:
    Clock* clock = nullptr;
    Bus* bus = nullptr;
    FFWrapper* ffWrapper = nullptr;
    Element* videoDecoder = nullptr;
    States states;
    VideoDevice* videoDevice = nullptr;
    void* surface = nullptr;
    std::thread renderingThread;
    Queue<Event> eventQueue;
private:
    struct BufferCompare {
        bool operator()(const AVFrame* lFrame, const AVFrame* rFrame);
    };
    PriorityQueue<AVFrame*, BufferCompare> bufferQueue;
};