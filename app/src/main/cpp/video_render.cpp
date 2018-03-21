#include "log.h"
#include "ffwrapper.h"
#include "video_render.h"
#include "video_device.h"

#undef  LOG_TAG 
#define LOG_TAG "VideoRender"


void VideoRender::rendering() {
    LOGD("rendering: thread started");
    bool pendingEOS = false;
    for (;;) {
        // Handle events
        Event ev;
        if (eventQueue.pop(ev)) {
            if (ev.id == EVENT_STOP_THREAD) {
                while (!bufferQueue.empty()) {
                    AVFrame* frame = nullptr;
                    bufferQueue.pop(frame);
                    ffWrapper->freeFrame(frame);
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

        // Current is PAUSE
        if (states.getCurrent() == PAUSED) {
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
        // Sync video displaying based on the given clock
        using namespace std::chrono;
        system_clock::time_point tp = system_clock::now();
        double offset = frame->pts * ffWrapper->videoTimeBase() * 1000 - clock->runningTime()/1000;
        double threshold = 500 / ffWrapper->videoFPS();
        if (offset < 0.0f) {
            // rendering speed is slower
            if (offset > -threshold) {
                LOGD("rendering: clock=%lldms, offset=%.6gms ( > %.6gms, write video fram)",
                     clock->runningTime()/1000, offset, -threshold);
                videoDevice->write(frame, sizeof(AVFrame));
            } else {
                LOGD("rendering: clock=%lldms, offset=%.6gms ( <= %.6gms, drop video frame)",
                     clock->runningTime()/1000, offset, -threshold);
                ffWrapper->freeFrame(frame);
            }
        } else {
            // rendering speed is faster
            if (offset > threshold) {
                int64_t sleepDuration = offset - threshold;
                // FIXME: if sleepDuration is too big, we shouldn't sleep too long duration.
                if (sleepDuration > threshold) {
                    LOGD("rendering: clock=%lldms, offset=%.6gms ( > %.6gms, write video frame)",
                         clock->runningTime()/1000, offset, 2*threshold);
                    videoDevice->write(frame, sizeof(AVFrame));
                } else {
                    LOGD("rendering: clock=%lldms, offset=%.6gms ( > %.6gms, sleep %lldms and write video frame)",
                         clock->runningTime()/1000, offset, sleepDuration, threshold);
                    std::this_thread::sleep_for(milliseconds(sleepDuration));
                    videoDevice->write(frame, sizeof(AVFrame));
                }
            } else {
                LOGD("rendering: clock=%lldms, offset=%.6gms ( <= %.6gms, write video frame)",
                     clock->runningTime()/1000, offset, threshold);
                videoDevice->write(frame, sizeof(AVFrame));
            }
        }
        system_clock::duration d = system_clock::now() - tp;
        LOGT("rendering: clock=%lldms, duration is %lldms",
             clock->runningTime()/1000, duration_cast<milliseconds>(d).count());
    }
}

VideoRender::VideoRender() {
    videoDevice = VideoDevice::create("SurfaceDevice"); 
}

VideoRender::~VideoRender() {    
    VideoDevice::release(videoDevice);
}

int VideoRender::toIdle() {
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

int VideoRender::toReady() {
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
        renderingThread.join();
        states.setCurrent(READY);
        return STATUS_SUCCESS;
    } 

    LOGE("toReady failed: current state is %s", cstr(current));
    return STATUS_FAILED;
}

int VideoRender::toPaused() {
    State current = states.getCurrent();
    if (current == PAUSED) {
        LOGW("toPaused: current state is PAUSED");
        return STATUS_SUCCESS;
    }

    if (current == READY) {
        renderingThread = std::thread(&VideoRender::rendering, this);
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

int VideoRender::toPlaying() {
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

int VideoRender::setState(State state) {
    typedef int (VideoRender::*ToStateFun)();
    ToStateFun toStates[] = {
            &VideoRender::toIdle,
            &VideoRender::toReady,
            &VideoRender::toPaused,
            &VideoRender::toPlaying
    };
    return (this->*toStates[state])();
}

int VideoRender::sendEvent(const Event& event) {
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

int VideoRender::pushBuffer(const Buffer& buffer) {
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

bool VideoRender::BufferCompare::operator()(const AVFrame* lFrame, const AVFrame* rFrame) {
    return lFrame->pts >= rFrame->pts;
};