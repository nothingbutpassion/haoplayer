#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include "clock.h"

#define AUDIO_ENGIN                 0x01
#define AUDIO_SAMPLE_RATE           0x02
#define AUDIO_SAMPLE_FORMAT         0x04
#define AUDIO_SAMPLE_BUFFER_SIZE    0x08


struct AudioDevice {
    static AudioDevice* create(const std::string& name);
    static void release(AudioDevice* device);
    virtual ~AudioDevice() {};
    virtual bool setProperty(int key, void* value) = 0;
    virtual int  getSampleRate() = 0;
    virtual int  getSampleFormat() = 0;
    virtual int  getChannels() = 0;
    virtual int  getPlaybackPosition() = 0;
    virtual int  write(void* buf, int buflen) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void flush() = 0;
    virtual void stop() = 0;
};

struct AudioDeviceClock: public Clock {

    AudioDeviceClock(AudioDevice* audioDevice) {
        this->audioDevice = audioDevice;
    }

    int64_t runningTime() override {
        int64_t sampleRate = audioDevice->getSampleRate();
        int64_t sampleFrames = audioDevice->getPlaybackPosition();
        int64_t running = 0;
        if (sampleRate != 0) {
            running = 1000000L*sampleFrames/sampleRate;
        }
        return offset.load() + running;
    }

    int64_t baseTime() override {
        return absoluteTime() - runningTime();
    }

    int64_t absoluteTime() override {
        using namespace std::chrono;
        system_clock::time_point tp = system_clock::now();
        system_clock::duration d = tp.time_since_epoch();
        return duration_cast<microseconds>(d).count();
    }

    void setOffset(uint64_t offsetTime) {
        offset.store(offsetTime);
    }

private:
    std::atomic<int64_t> offset;
    AudioDevice* audioDevice = nullptr;
};

