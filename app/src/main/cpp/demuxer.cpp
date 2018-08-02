#include "log.h"
#include "ffwrapper.h"
#include "demuxer.h"

#undef  LOG_TAG 
#define LOG_TAG "Demuxer"

Demuxer::Demuxer() {   
}

Demuxer::~Demuxer() {    
}

void Demuxer::demuxing() {
    LOGD("demuxing: thread started");
    std::vector<AVPacket> pendingPackets;
    bool isEOS = false;
    for (;;) {
        // Handle events
        Event ev;
        if (eventQueue.pop(ev)) {
            if (ev.id == EVENT_STOP_THREAD) {
                if (!pendingPackets.empty()) {
                    ffWrapper->freePacket(pendingPackets.front());
                    pendingPackets.clear();
                }
                LOGD("demuxing: thread exited");
                break;
            };
            continue;
        }

        // End of stream
        if (isEOS) {
            LOGD("demuxing: end of stream, will sleep 10ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Read a packet from container, or use the pending packet
        AVPacket packet;
        if (!pendingPackets.empty()) {
            if (states.getCurrent() == PAUSED) {
                LOGD("demuxing: current state is PAUSED, will sleep 10ms");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            packet = pendingPackets.front();
            pendingPackets.clear();
        } else {
            if (!ffWrapper->readPacket(packet, &isEOS)) {
                videoSink->sendEvent(Event(EVENT_EOS));
                audioSink->sendEvent(Event(EVENT_EOS));
                continue;
            }
        }

        // Push video/audio packets to corresponding sinks
        if (ffWrapper->isVideo(packet)) {
            // got video packet
            Buffer buf(BUFFER_AVPACKET, &packet);
            if (videoSink->pushBuffer(buf) == STATUS_FAILED) {
                LOGW("demuxing: push buffer to video decoder failed");
                pendingPackets.push_back(packet);
            }
        } else if (ffWrapper->isAudio(packet)) {
            // got audio packet
            Buffer buf(BUFFER_AVPACKET, &packet);
            if (audioSink->pushBuffer(buf) == STATUS_FAILED) {
                LOGW("demuxing: push buffer to audio decoder failed");
                pendingPackets.push_back(packet);
            }
        } else {
            ffWrapper->freePacket(packet);
        }
    }
}

int Demuxer::toIdle() {
    State current = states.getCurrent();
    if (current == IDLE) {
        LOGW("toReady: current state is IDLE");
        return STATUS_SUCCESS;
    }

    if (current == READY) {
        ffWrapper->close();
        states.setCurrent(IDLE);
        return STATUS_SUCCESS;
    }

    LOGE("toReady failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int Demuxer::toReady() {
    State current = states.getCurrent();
    if (current == READY) {
        LOGW("toReady: current state is READY");
        return STATUS_SUCCESS;
    }

    if (current == IDLE) {
        if (!ffWrapper->open(url.c_str())) {
            LOGE("toReady failed: can't open %s", url.c_str());
            bus->sendMessage(Message(MESSAGE_ERROR_SOURCE, this));
            return STATUS_FAILED;
        }
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    } 
    
    if (current == PAUSED) {
        sendEvent(Event(EVENT_STOP_THREAD));
        demuxingThread.join();
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    } 

    LOGE("toReady failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int Demuxer::toPaused() {
    State current = states.getCurrent();
    if (current == PAUSED) {
        LOGW("toReady: current state is PAUSED");
        return STATUS_SUCCESS;
    }

    if (current == READY) {
        demuxingThread = std::thread(&Demuxer::demuxing, this);
        states.setCurrent(PAUSED);
        return STATUS_SUCCESS;
    } 
    
    if (current == PLAYING) {
        states.setCurrent(PAUSED);
        return STATUS_SUCCESS;
    } 

    LOGE("toPaused failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int Demuxer::toPlaying() {
    State current = states.getCurrent();
    if (current != PAUSED) {
        LOGW("toPlaying: current state is PAUSED");
        return STATUS_FAILED;
    }
    states.setCurrent(PLAYING);
    return STATUS_SUCCESS;
}

int Demuxer::setState(State state) {
    typedef int (Demuxer::*ToStateFun)();
    ToStateFun toStates[] = {
        &Demuxer::toIdle,
        &Demuxer::toReady,
        &Demuxer::toPaused,
        &Demuxer::toPlaying
    }; 
    return (this->*toStates[state])();
}

int Demuxer::sendEvent(const Event& event) {
    State current = states.getCurrent();
    if (current == IDLE || current == READY) {
        LOGE("sendEvent failed: current state is %s", cstr(current));
        return STATUS_FAILED;
    }
    if (!eventQueue.push(event)) {
        LOGE("sendEvent failed: event queue is full, current state is %s", cstr(current));
        return STATUS_FAILED;
    }
    return STATUS_SUCCESS;
}


int Demuxer::pushBuffer(const Buffer& buffer) {
    LOGE("pushBuffer failed: not supported");
    return STATUS_FAILED; 
}

