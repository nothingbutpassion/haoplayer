#pragma once

#include "state.h"
#include "clock.h"
#include "bus.h"

#define EVENT_STOP_THREAD   0x01
#define EVENT_EOS           0x02

#define BUFFER_AVPACKET     0x01
#define BUFFER_AVFRAME      0x02

#define STATUS_FAILED       -1
#define STATUS_SUCCESS      0

struct Event {
    Event(int id, void* data) : id(id), data(data) {}
    Event(int id) : id(id) {}
    Event() {}
    int id = -1;
    void* data = nullptr;
};

struct Buffer {
    Buffer(int id, void* data) : id(id), data(data) {}
    Buffer(int id) : id(id) {}
    Buffer() {}
    int id = -1;
    void* data = nullptr;
};


struct Element {
    virtual void setBus(Bus* bus) = 0;
    
    virtual void setClock(Clock* clock) = 0;
    virtual Clock* getClock() = 0;

    virtual int setState(State state) = 0;
    virtual State getState() = 0;
    
    virtual int sendEvent(const Event& event) = 0;
    virtual int pushBuffer(const Buffer& buffer) = 0;
};