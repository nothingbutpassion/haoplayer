#include "log.h"
#include "ffwrapper.h"
#include "video_decoder.h"

#undef  LOG_TAG 
#define LOG_TAG "VideoDecoder"

void VideoDecoder::decoding() {
    LOGD("decoding: thread started");
    AVFrame* pendingFrame = nullptr;
    bool pendingEOS = false;
    for (;;) {
        // Handle events
        Event ev;
        if (eventQueue.pop(ev)) {
            if (ev.id == EVENT_STOP_THREAD) {
                if (pendingFrame) {
                    ffWrapper->freeFrame(pendingFrame);
                    pendingFrame = nullptr;
                }
                while (!bufferQueue.empty()) {
                    AVPacket p;
                    bufferQueue.pop(p);
                    ffWrapper->freePacket(p);
                }
                LOGD("decoding: thread exited");
                break;
            };
            if (ev.id == EVENT_FLUSH) {
                if (pendingFrame) {
                    ffWrapper->freeFrame(pendingFrame);
                    pendingFrame = nullptr;
                }
                while (!bufferQueue.empty()) {
                    AVPacket p;
                    bufferQueue.pop(p);
                    ffWrapper->freePacket(p);
                }
                LOGD("decoding: MESSAGE_FLUSH_FINISHED");
                bus->sendMessage(Message(MESSAGE_FLUSH_FINISHED, this));
                continue;
            }
            if (ev.id == EVENT_SEEK) {
                pendingEOS = false;
                LOGD("decoding: MESSAGE_FLUSH_FINISHED");
                bus->sendMessage(Message(MESSAGE_SEEK_FINISHED, this));
                continue;
            }
            if (ev.id == EVENT_EOS) {
                pendingEOS = true;
                videoSink->sendEvent(Event(EVENT_EOS));
                continue;
            }
            continue;
        }

        // Handle buffers
        AVFrame* frame = nullptr;
        if (pendingFrame) {
            frame = pendingFrame;
            pendingFrame = nullptr;
        } else {
            AVPacket packet;
            if (!bufferQueue.pop(packet)) {
                if (pendingEOS) {
                    LOGD("decoding: end of stream, will sleep 10ms");
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                if (states.getCurrent() == PAUSED) {
                    LOGD("decoding: current state is PAUSED, will sleep 10ms");
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                continue;
            }
            if (!ffWrapper->decodeVideo(packet, &frame, nullptr)) {
                LOGE("decoding: decode video error");
                bus->sendMessage(Message(MESSAGE_ERROR_DECODE, this));
                ffWrapper->freePacket(packet);
                continue;
            }
            ffWrapper->freePacket(packet);
        }
        // push buffer to video render
        Buffer buf(BUFFER_AVFRAME, frame);
        if (videoSink->pushBuffer(buf) == STATUS_FAILED) {
            LOGW("decoding: push buffer to video render failed");
            pendingFrame = frame;
        }
    }
}

VideoDecoder::VideoDecoder() {   
}

VideoDecoder::~VideoDecoder() {    
}

int VideoDecoder::toIdle() {
    State current = states.getCurrent();
    if (current == IDLE) {
        LOGW("toIdle: current state is IDLE");
        return STATUS_SUCCESS;
    }

    if (current != READY) {
        LOGE("toIdle failed: current state is %s", cstr(current));
        return STATUS_FAILED;  
    }

    states.setCurrent(IDLE);
    return STATUS_SUCCESS;
}

int VideoDecoder::toReady() {
    State current = states.getCurrent();
    if (current == READY) {
        LOGW("toReady: current state is READY");
        return STATUS_SUCCESS;
    }

    if (current == IDLE) {
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    } 
    
    if (current == PAUSED) {
        sendEvent(EVENT_STOP_THREAD);
        decodingThread.join();
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    } 

    LOGE("toReady failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int VideoDecoder::toPaused() {
    State current = states.getCurrent();
    if (current == PAUSED) {
        LOGW("toReady: current state is PAUSED");
        return STATUS_SUCCESS;
    }

    if (current == READY) {
        decodingThread = std::thread(&VideoDecoder::decoding, this);
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

int VideoDecoder::toPlaying() {
    State current = states.getCurrent();
    if (current == PLAYING) {
        LOGW("toPlaying: current state is PLAYING");
        return STATUS_SUCCESS;
    }

    if (current != PAUSED) {
        LOGE("toPlaying failed: current state is %s", cstr(current));
        return STATUS_FAILED;
    }

    states.setCurrent(PLAYING);
    return STATUS_SUCCESS;
}

int VideoDecoder::setState(State state) {
    typedef int (VideoDecoder::*ToStateFun)();
    ToStateFun toStates[] = {
        &VideoDecoder::toIdle,
        &VideoDecoder::toReady,
        &VideoDecoder::toPaused,
        &VideoDecoder::toPlaying
    }; 
    return (this->*toStates[state])();
}

int VideoDecoder::sendEvent(const Event& event) {
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


int VideoDecoder::pushBuffer(const Buffer& buffer) {
    if (states.getCurrent() == IDLE) {
        LOGE("pushBuffer failed: current state is IDLE");
        return STATUS_FAILED;
    }

    if (!bufferQueue.push(*static_cast<AVPacket*>(buffer.data))) {
        LOGE("pushBuffer failed: buffer queue is full");
        return STATUS_FAILED;
    }
    return STATUS_SUCCESS;
}


bool VideoDecoder::BufferCompare::operator()(const AVPacket& lPacket, const AVPacket& rPacket) {
    return lPacket.dts >= rPacket.dts;
};