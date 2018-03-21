#include "log.h"
#include "ffwrapper.h"
#include "audio_decoder.h"

#undef  LOG_TAG 
#define LOG_TAG "AudioDecoder"


void AudioDecoder::decoding() {
    LOGD("decoding: thread stated");
    AVFrame* pendingFrame = nullptr;
    bool pendingEOS = false;
    for (;;) {
        // Handle events
        Event ev;
        if (eventQueue.pop(ev)) {
            if (ev.id == EVENT_STOP_THREAD) {
                if (pendingFrame) {
                    ffWrapper->freeFrame(pendingFrame);
                }
                while (!bufferQueue.empty()) {
                    AVPacket p;
                    bufferQueue.pop(p);
                    ffWrapper->freePacket(p);
                }
                LOGD("demuxing: thread exited");
                break;
            };
            if (ev.id == EVENT_EOS) {
                pendingEOS = true;
                audioSink->sendEvent(Event(EVENT_EOS));
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
            // Should loop decoding audio
            if (!ffWrapper->decodeAudio(packet, &frame, nullptr)) {
                LOGE("decoding: decode audio error");
                bus->sendMessage(Message(MESSAGE_ERROR_DECODE, this));
                ffWrapper->freePacket(packet); 
                continue;
            }
            ffWrapper->freePacket(packet);
        }
        // push buffer to audio render
        Buffer buf(BUFFER_AVFRAME, frame);
        if (audioSink->pushBuffer(buf) == STATUS_FAILED) {
            LOGW("decoding: push buffer to audio render failed");
            pendingFrame = frame;
        }
    }
}

AudioDecoder::AudioDecoder() {   
}

AudioDecoder::~AudioDecoder() {    
}

int AudioDecoder::toIdle() {
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

int AudioDecoder::toReady() {
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

int AudioDecoder::toPaused() {
    State current = states.getCurrent();
    if (current == PAUSED) {
        LOGW("toReady: current state is PAUSED");
        return STATUS_SUCCESS;
    }

    if (current == READY) {
        decodingThread = std::thread(&AudioDecoder::decoding, this);
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

int AudioDecoder::toPlaying() {
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

int AudioDecoder::setState(State state) {
    typedef int (AudioDecoder::*ToStateFun)();
    ToStateFun toStates[] = {
        &AudioDecoder::toIdle,
        &AudioDecoder::toReady,
        &AudioDecoder::toPaused,
        &AudioDecoder::toPlaying
    }; 
    return (this->*toStates[state])();
}

int AudioDecoder::sendEvent(const Event& event) {
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


int AudioDecoder::pushBuffer(const Buffer& buffer) {
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


bool AudioDecoder::BufferCompare::operator()(const AVPacket& lPacket, const AVPacket& rPacket) {
    return lPacket.dts >= rPacket.dts;
};