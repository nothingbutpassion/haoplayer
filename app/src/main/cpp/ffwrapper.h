#pragma once

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class FFWrapper {
public:
    // class utils
    static void freePacket(AVPacket& packet);
    static void freeFrame(AVFrame* frame);
    static SwsContext* getVideoScale(
        int src_w, int src_h, AVPixelFormat src_pix_fmt,
        int dst_w, int dst_h, AVPixelFormat dst_pix_fmt);
    static void freeVideoScale(SwsContext* videoScaleContext);
    static void scaleVideo(SwsContext* videoScaleContext, const AVFrame* frame, 
        uint8_t** dst_data, int* dst_linesize);
    static SwrContext* getAudioResample(
        int64_t src_ch_layout, int src_rate, AVSampleFormat src_sample_fmt, 
        int64_t dst_ch_layout, int dst_rate, AVSampleFormat dst_sample_fmt);
    static void freeAudioResample(SwrContext* audioResampleContext);
    static void resampleAudio(SwrContext* audioResampleContext, 
        const AVFrame* frame, uint8_t** dst_data, int dst_samples);
public:
    FFWrapper();
    ~FFWrapper();
    bool open(const char* url);
    void close();
    // NOTES: in AV_TIME_BASE fractional seconds
    bool seek(int64_t timestamp);
    bool readPacket(AVPacket& packet, bool* isEOF = nullptr);

    // video related
    bool decodeVideo(const AVPacket& packet, AVFrame** frame, int* decoded = nullptr);
    bool setVideoScale(const AVFrame* frame, int dst_w, int dst_h, 
        AVPixelFormat dst_pix_fmt);                   
    void scaleVideo(const AVFrame* frame, uint8_t** dst_data, int* dst_linesize);
    
    // audio related
    bool decodeAudio(const AVPacket& packet, AVFrame** frame, int* decoded = nullptr);
    bool setAudioResample(const AVFrame* frame, int64_t dst_ch_layout, 
        int dst_rate, AVSampleFormat dst_sample_fmt);
    void resampleAudio(const AVFrame* frame, uint8_t** dst_data, int dst_samples);

public:
    // NOTES: in AV_TIME_BASE fractional seconds
    int64_t startTime() {
        return formatContext->start_time;
    }
    // NOTES: in AV_TIME_BASE fractional seconds
    int64_t duration() {
        return formatContext->duration;
    }

    // video related
    bool isVideo(const AVPacket& packet) {
        return packet.stream_index == videoIndex;
    }
    const char* videoPixelFormat() {
        return av_get_pix_fmt_name(videoCodecContext->pix_fmt);
    }
    int videoWidth() {
        return videoCodecContext->width;
    }
    int videoHeight() {
        return videoCodecContext->height;
    }
    double videoTimeBase() {
        return av_q2d(formatContext->streams[videoIndex]->time_base);
    }
    int64_t videoStartTime() {
        return formatContext->streams[videoIndex]->start_time;
    }
    int64_t videoDuration() {
        return formatContext->streams[videoIndex]->duration;
    }
    int64_t videoFrames() {
        return formatContext->streams[videoIndex]->nb_frames;
    }
    double videoFPS() {
        return av_q2d(formatContext->streams[videoIndex]->avg_frame_rate);
    }

    // audio related
    bool isAudio(const AVPacket& packet) {
        return packet.stream_index == audioIndex;
    }
    double audioTimeBase() {
        return av_q2d(formatContext->streams[audioIndex]->time_base);
    }
    int64_t audioStartTime() {
        return formatContext->streams[audioIndex]->start_time;
    }
    int64_t audioDuration() {
        return formatContext->streams[audioIndex]->duration;
    }
    const char* audioSampleFormat() {
        return av_get_sample_fmt_name(audioCodecContext->sample_fmt);
    }
    int audioChannels() {
        return audioCodecContext->channels;
    }
    int audioSampleRate() {
        return audioCodecContext->sample_rate;
    }


private:
    AVFormatContext* formatContext = nullptr;
    
    // video related
    int videoIndex = -1;
    AVCodecContext* videoCodecContext = nullptr;
    AVFrame* videoFrame = nullptr;
    SwsContext* videoScaleContext = nullptr;

    // audio related
    int audioIndex = -1;
    AVCodecContext* audioCodecContext = nullptr;
    AVFrame* audioFrame = nullptr;
    SwrContext* audioResampleContext = nullptr;
};