#include "log.h"
#include "ffwrapper.h"
#include "audio_track.h"
#include "audio_device.h"

#undef  LOG_TAG
#define LOG_TAG "AudioTrackDevice"

class AudioTrackDevice: public AudioDevice {
public:

    AudioTrackDevice() {
        sampleBuffer = new uint8_t[sampleBufferSize];
    }

    ~AudioTrackDevice() {
        delete sampleBuffer;
        if (audioTrack) {
            releaseAudioTrack(audioTrack);
            deleteAudioTrack(audioTrack);
        }
    }

    bool setProperty(int key, void* value) override {
        switch(key) {
        case AUDIO_ENGIN:
            ffWrapper = static_cast<FFWrapper*>(value);
            break;
        case AUDIO_SAMPLE_BUFFER_SIZE:
            sampleBufferSize = *static_cast<int*>(value);
            break;
        case AUDIO_SAMPLE_RATE:
            sampleRate = *static_cast<int*>(value);
            audioTrack = newAudioTrack(sampleRate);
            break;
        case AUDIO_SAMPLE_FORMAT:
            if (*static_cast<int*>(value) != AV_SAMPLE_FMT_S16) {
                LOGW("setProperty: only AV_SAMPLE_FMT_S16 is supported");
            }
            break;
        default:
            return false;
        }
        return true;
    }

    int getSampleRate() override {
        return sampleRate;
    }

    int getSampleFormat() override {
        return AV_SAMPLE_FMT_S16;
    }

    int getChannels() override {
        return 2;
    }

    int getPlaybackPosition() override {
        return getAudioTrackPosition(audioTrack);
    }

    int write(void* buf, int buflen) override {
        AVFrame* frame = static_cast<AVFrame*>(buf);
        if (sampleRate != frame->sample_rate || sampleForamt != frame->format) {
            ffWrapper->setAudioResample(frame, AV_CH_LAYOUT_STEREO, frame->sample_rate, AV_SAMPLE_FMT_S16);
            sampleRate = frame->sample_rate;
            sampleForamt = frame->format;
        }

        int sampleSize = 2 * 2 * frame->nb_samples;
        if (sampleBufferSize < sampleSize) {
            delete sampleBuffer;
            sampleBuffer = new uint8_t[sampleSize];
            sampleBufferSize = sampleSize;
        }

        ffWrapper->resampleAudio(frame, &sampleBuffer, frame->nb_samples);
        ffWrapper->freeFrame(frame);
        return writeAudioTrack(audioTrack, sampleBuffer, sampleSize);
    }

    void play() override {
        if (getAudioTrackPlayState(audioTrack) != PLAYSTATE_PLAYING) {
            playAudioTrack(audioTrack);
        }
    }

    void pause() override {
        if (getAudioTrackPlayState(audioTrack) == PLAYSTATE_PLAYING) {
            pauseAudioTrack(audioTrack);
        }
    }

    void flush() override {
        flushAudioTrack(audioTrack);
    }

    void stop() override {
        stopAudioTrack(audioTrack);
    }
private:
    int sampleRate = 0;
    int sampleForamt = AV_SAMPLE_FMT_NONE;
    uint8_t* sampleBuffer = nullptr;
    int sampleBufferSize = 256*1024;
    FFWrapper* ffWrapper = nullptr;
    jobject audioTrack = nullptr;
};

AudioDevice* AudioDevice::create(const std::string& name) {
    if (name != "AudioTrackDevice") {
        return nullptr;
    }
    return new AudioTrackDevice;
}

void AudioDevice::release(AudioDevice* device) {
    delete device;
}