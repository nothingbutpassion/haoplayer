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
                audioSink->onEvent(Event(EVENT_EOS));
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
        if (audioSink->onBuffer(buf) == STATUS_FAILED) {
            LOGW("decoding: push buffer to audio render failed");
            pendingFrame = frame;
        }
    }
}

AudioDecoder::AudioDecoder() {   
}

AudioDecoder::~AudioDecoder() {    
}

int AudioDecoder::toNull() {
    State current = states.getCurrent();
    if (current == STATE_NULL) {
        LOGW("toNull: current state is STATE_NULL");
        return STATUS_SUCCESS;
    }

    if (current != STATE_READY) {
        LOGE("toNull failed: current state is %s", cstr(current));
        return STATUS_FAILED;  
    }

    states.setCurrent(STATE_NULL);
    return STATUS_SUCCESS;
}

int AudioDecoder::toReady() {
    State current = states.getCurrent();
    if (current == STATE_READY) {
        LOGW("toReady: current state is STATE_READY");
        return STATUS_SUCCESS;
    }

    if (current == STATE_NULL) {
        states.setCurrent(STATE_READY);
        return STATUS_SUCCESS;
    } 
    
    if (current == STATE_PAUSED) {
        onEvent(EVENT_STOP_THREAD);
        decodingThread.join();
        states.setCurrent(STATE_READY);
        return STATUS_SUCCESS;
    } 

    LOGE("toReady failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int AudioDecoder::toPaused() {
    State current = states.getCurrent();
    if (current == STATE_PAUSED) {
        LOGW("toReady: current state is STATE_PAUSED");
        return STATUS_SUCCESS;
    }

    if (current == STATE_READY) {
        decodingThread = std::thread(&AudioDecoder::decoding, this);
        states.setCurrent(STATE_PAUSED);
        return STATUS_SUCCESS;
    } 
    
    if (current == STATE_PLAYING) {
        states.setCurrent(STATE_PAUSED);
        return STATUS_SUCCESS;
    } 

    LOGE("toPaused failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int AudioDecoder::toPlaying() {
    State current = states.getCurrent();
    if (current == STATE_PLAYING) {
        LOGW("toPlaying: current state is STATE_PLAYING");
        return STATUS_SUCCESS;
    }

    if (current != STATE_PAUSED) {
        LOGE("toPlaying failed: current state is %s", cstr(current));
        return STATUS_FAILED;
    }

    states.setCurrent(STATE_PLAYING);
    return STATUS_SUCCESS;
}

int AudioDecoder::setState(State state) {
    typedef int (AudioDecoder::*ToStateFun)();
    ToStateFun toStates[] = {
        &AudioDecoder::toNull,
        &AudioDecoder::toReady,
        &AudioDecoder::toPaused,
        &AudioDecoder::toPlaying
    }; 
    return (this->*toStates[state])();
}

int AudioDecoder::onEvent(const Event& event) {
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


int AudioDecoder::onBuffer(const Buffer& buffer) {
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


bool AudioDecoder::BufferCompare::operator()(const AVPacket& lPacket, const AVPacket& rPacket) {
    return lPacket.dts >= rPacket.dts;
};