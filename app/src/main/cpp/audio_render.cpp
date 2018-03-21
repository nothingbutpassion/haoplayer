#include "log.h"
#include "ffwrapper.h"
#include "audio_render.h"

#undef  LOG_TAG 
#define LOG_TAG "AudioRender"

void AudioRender::rendering() {
    LOGD("rendering: thread stated");
    bool pendingEOS = false;
    bool firstFrame = true;
    for (;;) {
        // Handle events
        Event ev;
        if (eventQueue.pop(ev)) {
            if (ev.id == EVENT_STOP_THREAD) {
                audioDevice->flush();
                audioDevice->stop();
                while (!bufferQueue.empty()) {
                    AVFrame* f = nullptr;
                    bufferQueue.pop(f);
                    ffWrapper->freeFrame(f);
                }
                LOGD("rendering: thread exited");
                break;
            };
            if (ev.id == EVENT_EOS) {
                pendingEOS = true;
                continue;
            }
            continue;
        }

        // Current is PAUSED
        if (states.getCurrent() == PAUSED) {
            audioDevice->pause();
            LOGD("rendering: current state is PAUSED, will sleep 10ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Current is PLAYING
        AVFrame* frame = nullptr;
        if (!bufferQueue.pop(frame)) {
            if (pendingEOS) {
                LOGD("rendering: end of stream, will sleep 10ms");
                bus->sendMessage(Message(MESSAGE_EOS));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            LOGW("rendering, buffer queue is empty");
            continue;
        }
        // Output the audio stream
        if (firstFrame) {
            firstFrame = false;
            static_cast<AudioDeviceClock*>(clock)->setOffset(frame->pts * ffWrapper->audioTimeBase() * 1000000);
        }
        audioDevice->play();
        audioDevice->write(frame, sizeof(AVFrame));
        LOGD("rendering: clock running time is %lld", clock->runningTime()/1000);
    }
}

AudioRender::AudioRender() {
    audioDevice = AudioDevice::create("AudioTrackDevice");
    clock = new AudioDeviceClock(audioDevice); 
}

AudioRender::~AudioRender() {
    delete clock;
    delete audioDevice;
}

int AudioRender::toIdle() {
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

int AudioRender::toReady() {
    State current = states.getCurrent();
    if (current == READY) {
        LOGW("toReady: current state is READY");
        return STATUS_SUCCESS;
    }

    if (current == IDLE) {
        int audioSampleRate = ffWrapper->audioSampleRate();
        audioDevice->setProperty(AUDIO_SAMPLE_RATE, &audioSampleRate);
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    }
    
    if (current == PAUSED) {
        sendEvent(EVENT_STOP_THREAD);
        renderingThread.join();
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    }

    LOGE("toReady failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int AudioRender::toPaused() {
    State current = states.getCurrent();
    if (current == PAUSED) {
        LOGW("toPaused: current state is PAUSED");
        return STATUS_SUCCESS;
    }

    if (current == READY) {
        renderingThread = std::thread(&AudioRender::rendering, this);
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

int AudioRender::toPlaying() {
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

int AudioRender::setState(State state) {
    typedef int (AudioRender::*ToStateFun)();
    ToStateFun toStates[] = {
        &AudioRender::toIdle,
        &AudioRender::toReady,
        &AudioRender::toPaused,
        &AudioRender::toPlaying
    }; 
    return (this->*toStates[state])();
}

int AudioRender::sendEvent(const Event& event) {
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

int AudioRender::pushBuffer(const Buffer& buffer) {
    State current = states.getCurrent();
    if (current == IDLE || current == READY) {
        LOGE("pushBuffer failed: current state is %s", cstr(current));
        return STATUS_FAILED;
    }
    AVFrame* frame = static_cast<AVFrame*>(buffer.data);
    if (!bufferQueue.push(frame)) {
        LOGE("pushBuffer failed: buffer queue is full");
        return STATUS_FAILED;  
    }
    return STATUS_SUCCESS;
}

bool AudioRender::BufferCompare::operator()(const AVFrame* lFrame, const AVFrame* rFrame) {
    return lFrame->pts >= rFrame->pts;
};