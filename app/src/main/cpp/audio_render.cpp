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

        // Current is STATE_PAUSED
        if (states.getCurrent() == STATE_PAUSED) {
            audioDevice->pause();
            LOGD("rendering: current state is STATE_PAUSED, will sleep 10ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Current is STATE_PLAYING
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

int AudioRender::toNull() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_NULL)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    states.setCurrent(STATE_NULL);
    return STATUS_SUCCESS;
}

int AudioRender::toReady() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_READY)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    if (current == STATE_NULL) {
        int audioSampleRate = ffWrapper->audioSampleRate();
        audioDevice->setProperty(AUDIO_SAMPLE_RATE, &audioSampleRate);
        states.setCurrent(STATE_READY);
        return STATUS_SUCCESS;
    }
    // current == STATE_PAUSED
    onEvent(EVENT_STOP_THREAD);
    renderingThread.join();
    states.setCurrent(STATE_READY);
    return STATUS_SUCCESS;
}

int AudioRender::toPaused() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_PAUSED)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    if (current == STATE_READY) {
        renderingThread = std::thread(&AudioRender::rendering, this);
        states.setCurrent(STATE_PAUSED);
        return STATUS_SUCCESS;
    }
    // current == STATE_PLAYING
    states.setCurrent(STATE_PAUSED);
    return STATUS_SUCCESS;
}

int AudioRender::toPlaying() {
    State current = states.getCurrent();
    if (!checkState(current, STATE_PLAYING)) {
        LOGE("%s failed: current state is %s", __func__, cstr(current));
        return STATUS_FAILED;
    }
    states.setCurrent(STATE_PLAYING);
    return STATUS_SUCCESS;
}

int AudioRender::setState(State state) {
    typedef int (AudioRender::*ToStateFun)();
    ToStateFun toStates[] = {
        &AudioRender::toNull,
        &AudioRender::toReady,
        &AudioRender::toPaused,
        &AudioRender::toPlaying
    }; 
    return (this->*toStates[state])();
}

int AudioRender::onEvent(const Event& event) {
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

int AudioRender::onBuffer(const Buffer& buffer) {
    State current = states.getCurrent();
    if (current == STATE_NULL || current == STATE_READY) {
        LOGE("onBuffer failed: current state is %s", cstr(current));
        return STATUS_FAILED;
    }
    AVFrame* frame = static_cast<AVFrame*>(buffer.data);
    if (!bufferQueue.push(frame)) {
        LOGE("onBuffer failed: buffer queue is full");
        return STATUS_FAILED;  
    }
    return STATUS_SUCCESS;
}

bool AudioRender::BufferCompare::operator()(const AVFrame* lFrame, const AVFrame* rFrame) {
    return lFrame->pts >= rFrame->pts;
};