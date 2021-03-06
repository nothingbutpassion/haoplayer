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
            if (ev.id == EVENT_EOS) {
                pendingEOS = true;
                videoSink->onEvent(Event(EVENT_EOS));
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
                if (states.getCurrent() == STATE_PAUSED) {
                    LOGD("decoding: current state is STATE_PAUSED, will sleep 10ms");
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
        if (videoSink->onBuffer(buf) == STATUS_FAILED) {
            LOGW("decoding: push buffer to video render failed");
            pendingFrame = frame;
        }
    }
}

VideoDecoder::VideoDecoder() {   
}

VideoDecoder::~VideoDecoder() {    
}

int VideoDecoder::toNull() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_NULL)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    states.setCurrent(STATE_NULL);
    return STATUS_SUCCESS;
}

int VideoDecoder::toReady() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_READY)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    if (current == STATE_NULL) {
        states.setCurrent(STATE_READY);
        return STATUS_SUCCESS;
    }
    // current == STATE_PAUSED
    onEvent(EVENT_STOP_THREAD);
    decodingThread.join();
    states.setCurrent(STATE_READY);
    return STATUS_SUCCESS;
}

int VideoDecoder::toPaused() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_PAUSED)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    if (current == STATE_READY) {
        decodingThread = std::thread(&VideoDecoder::decoding, this);
        states.setCurrent(STATE_PAUSED);
        return STATUS_SUCCESS;
    }
    // current == STATE_PLAYING)
    states.setCurrent(STATE_PAUSED);
    return STATUS_SUCCESS;
}

int VideoDecoder::toPlaying() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_PLAYING)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    states.setCurrent(STATE_PLAYING);
    return STATUS_SUCCESS;
}

int VideoDecoder::setState(State state) {
    typedef int (VideoDecoder::*ToStateFun)();
    ToStateFun toStates[] = {
        &VideoDecoder::toNull,
        &VideoDecoder::toReady,
        &VideoDecoder::toPaused,
        &VideoDecoder::toPlaying
    }; 
    return (this->*toStates[state])();
}

int VideoDecoder::onEvent(const Event& event) {
    State current = states.getCurrent();
    if (current == STATE_NULL || current == STATE_READY) {
        LOGE("onEvent failed: current state is %s", cstr(current));
        return STATUS_FAILED;
    }

    if (!eventQueue.push(event)) {
        LOGE("onEvent failed: event queue is full, current state is %s", cstr(current));
        return STATUS_FAILED;
    }
    return STATUS_SUCCESS;
}


int VideoDecoder::onBuffer(const Buffer& buffer) {
    if (states.getCurrent() == STATE_NULL) {
        LOGE("onBuffer failed: current state is STATE_NULL");
        return STATUS_FAILED;
    }

    if (!bufferQueue.push(*static_cast<AVPacket*>(buffer.data))) {
        LOGE("onBuffer failed: buffer queue is full");
        return STATUS_FAILED;
    }
    return STATUS_SUCCESS;
}


bool VideoDecoder::BufferCompare::operator()(const AVPacket& lPacket, const AVPacket& rPacket) {
    return lPacket.dts >= rPacket.dts;
};