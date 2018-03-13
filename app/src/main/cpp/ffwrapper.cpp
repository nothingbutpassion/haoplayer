#include "log.h"
#include "ffwrapper.h"

#undef  LOG_TAG 
#define LOG_TAG "FFWrapper"

//
// class utils
//
void FFWrapper::freePacket(AVPacket& packet) {
    av_packet_unref(&packet);
    LOGD("freePacket ok");
}

void FFWrapper::freeFrame(AVFrame* frame) {
    av_frame_free(&frame);
    LOGD("freeFrame ok");
}

SwsContext* FFWrapper::getVideoScale(int src_w, int src_h, AVPixelFormat src_pix_fmt,
    int dst_w, int dst_h, AVPixelFormat dst_pix_fmt) {
    SwsContext* videoScaleContext = sws_getContext(
        src_w, src_h, src_pix_fmt, 
        dst_w, dst_h, dst_pix_fmt, 
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!videoScaleContext) {
        LOGE("sws_getContext failed: src_size=%dx%d, src_fmt=%s, dst_size=%dx%d, dst_fmt=%s",         
            src_w, src_h, av_get_pix_fmt_name((AVPixelFormat)src_pix_fmt), 
            dst_w, dst_h, av_get_pix_fmt_name((AVPixelFormat)dst_pix_fmt));
    } else {
        LOGD("sws_getContext succeed: src_size=%dx%d, src_fmt=%s, dst_size=%dx%d, dst_fmt=%s",         
            src_w, src_h, av_get_pix_fmt_name((AVPixelFormat)src_pix_fmt), 
            dst_w, dst_h, av_get_pix_fmt_name((AVPixelFormat)dst_pix_fmt)); 
    }
    return videoScaleContext;
}

void FFWrapper::freeVideoScale(SwsContext* videoScaleContext) {
    if (videoScaleContext) {
        sws_freeContext(videoScaleContext);
    }
}

void FFWrapper::scaleVideo(SwsContext* videoScaleContext, const AVFrame* frame, uint8_t** dst_data, int* dst_linesize) {
    sws_scale(videoScaleContext, (const uint8_t* const*)frame->data, 
        frame->linesize, 0, frame->height, dst_data, dst_linesize);
}


SwrContext* FFWrapper::getAudioResample(int64_t src_ch_layout, int src_rate, AVSampleFormat src_sample_fmt,
                                        int64_t dst_ch_layout, int dst_rate, AVSampleFormat dst_sample_fmt) {
    SwrContext* audioResampleContext = swr_alloc();
    if (!audioResampleContext) {
        LOGE("swr_alloc failed");
        return audioResampleContext;
    }
    // set options
    av_opt_set_int(audioResampleContext, "in_channel_layout",       src_ch_layout, 0);
    av_opt_set_int(audioResampleContext, "in_sample_rate",          src_rate, 0);
    av_opt_set_sample_fmt(audioResampleContext, "in_sample_fmt",    src_sample_fmt, 0);
    av_opt_set_int(audioResampleContext, "out_channel_layout",      dst_ch_layout, 0);
    av_opt_set_int(audioResampleContext, "out_sample_rate",         dst_rate, 0);
    av_opt_set_sample_fmt(audioResampleContext, "out_sample_fmt",   dst_sample_fmt, 0);

    // initialize the resampling context
    if (swr_init(audioResampleContext) < 0) {
        LOGE("swr_init failed");
        swr_free(&audioResampleContext);
    }
    return audioResampleContext;
}
void FFWrapper::freeAudioResample(SwrContext* audioResampleContext) {
    if (audioResampleContext) {
        swr_free(&audioResampleContext);
    }
}
void FFWrapper::resampleAudio(SwrContext* audioResampleContext, const AVFrame* frame, 
                              uint8_t** dst_data, int dst_samples) {
    swr_convert(audioResampleContext, 
        dst_data, dst_samples, 
        (const uint8_t**)frame->extended_data, frame->nb_samples);
}


//
// FFWrapper implementation
//
FFWrapper::FFWrapper() {
    // register all formats and codecs
    av_register_all();

    // temply save decoded video/audio frames
    videoFrame = av_frame_alloc();
    audioFrame = av_frame_alloc();
}

FFWrapper::~FFWrapper() {
    av_frame_free(&videoFrame);
    av_frame_free(&audioFrame);
    if (videoScaleContext) {
        sws_freeContext(videoScaleContext);
    }
    if (audioResampleContext) {
        swr_free(&audioResampleContext);
    }
    close();
}


bool FFWrapper::readPacket(AVPacket& packet) {
    // initialize packet, set data to nullptr, let the demuxer fill it
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;
    if (av_read_frame(formatContext, &packet) < 0) {
        LOGE("av_read_frame failed");
        return false;
    }
    LOGD("readPacket ok");
    return true;
}

bool FFWrapper::decodeVideo(const AVPacket& packet, AVFrame** outframe, int* decoded) {
    int got_frame = 0;
    int ret = avcodec_decode_video2(videoCodecContext, videoFrame, &got_frame, &packet);
    if (ret < 0) {
        LOGE("avcodec_decode_video2 failed: %d", ret);
        return false; 
    }

    if (decoded) {
        *decoded = ret;
    }

    if (!got_frame) {
        LOGE("avcodec_decode_video2 can't got frame");
        return false; 
    }

    LOGD("got video frame: pix_fmt=%s, video_size=%dx%d, pts=%.6g", 
        av_get_pix_fmt_name((AVPixelFormat)videoFrame->format), videoFrame->width, videoFrame->height,
        av_q2d(formatContext->streams[videoIndex]->time_base)*videoFrame->pts);

    if (outframe) {
        // create a new frame that references the same data as videoFrame.
        // shortcut for av_frame_alloc()+av_frame_ref(). 
        *outframe = av_frame_clone(videoFrame);
    }
    return true;
}



bool FFWrapper::setVideoScale(const AVFrame* frame, int dst_w, int dst_h, AVPixelFormat dst_pix_fmt) {
    if (videoScaleContext) {
        LOGW("videoScaleContext is not nullptr, free it first");
        sws_freeContext(videoScaleContext);
    }
    videoScaleContext = sws_getContext(
        frame->width, frame->height, (AVPixelFormat)frame->format, 
        dst_w, dst_h, dst_pix_fmt, 
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!videoScaleContext) {
        LOGE("sws_getContext failed: src_size=%dx%d, src_fmt=%s, dst_size=%dx%d, dst_fmt=%s",         
            frame->width, frame->height, av_get_pix_fmt_name((AVPixelFormat)frame->format), 
            dst_w, dst_h, av_get_pix_fmt_name((AVPixelFormat)dst_pix_fmt));
        return false;
    }  
    LOGD("sws_getContext succeed: src_size=%dx%d, src_fmt=%s, dst_size=%dx%d, dst_fmt=%s",         
        frame->width, frame->height, av_get_pix_fmt_name((AVPixelFormat)frame->format), 
        dst_w, dst_h, av_get_pix_fmt_name((AVPixelFormat)dst_pix_fmt)); 
    return true;  
}

void FFWrapper::scaleVideo(const AVFrame* frame, uint8_t** dst_data, int* dst_linesize) {
    sws_scale(videoScaleContext, (const uint8_t* const*)frame->data, 
        frame->linesize, 0, frame->height, dst_data, dst_linesize);
}


bool FFWrapper::decodeAudio(const AVPacket& packet, AVFrame** outframe, int* decoded) {
    int got_frame = 0;
    int ret = avcodec_decode_audio4(audioCodecContext, audioFrame, &got_frame, &packet);
    if (ret < 0) {
        LOGE("avcodec_decode_audio4 failed: %d", ret);
        return false;
    }

    if (decoded) {
        *decoded = ret > packet.size ? packet.size : ret;
    }

    if (!got_frame) {
        LOGE("avcodec_decode_audio4 can't got frame");
        return false; 
    }

    LOGD("got audio frame: channels=%d, nb_samples=%d, pts=%.6g", 
         audioFrame->channels, audioFrame->nb_samples,
         av_q2d(formatContext->streams[audioIndex]->time_base)*audioFrame->pts);

    if (outframe) {
        // shortcut for av_frame_alloc()+av_frame_ref()
        *outframe = av_frame_clone(audioFrame);
    }
    return true;
}


bool FFWrapper::setAudioResample(const AVFrame* frame, int64_t dst_ch_layout, 
                      int dst_rate, AVSampleFormat dst_sample_fmt) {
    if (audioResampleContext) {
        LOGW("audioResampleContext is not nullptr, free it first");
        swr_free(&audioResampleContext);
    }

    audioResampleContext = swr_alloc();
    if (!audioResampleContext) {
        LOGE("swr_alloc failed");
        return false;
    }
    // set options
    av_opt_set_int(audioResampleContext, "in_channel_layout",     frame->channel_layout, 0);
    av_opt_set_int(audioResampleContext, "in_sample_rate",        frame->sample_rate, 0);
    av_opt_set_sample_fmt(audioResampleContext, "in_sample_fmt",  (AVSampleFormat)frame->format, 0);
    av_opt_set_int(audioResampleContext, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(audioResampleContext, "out_sample_rate",       dst_rate, 0);
    av_opt_set_sample_fmt(audioResampleContext, "out_sample_fmt", dst_sample_fmt, 0);

    // initialize the resampling context
    if (swr_init(audioResampleContext) < 0) {
        LOGE("swr_init failed");
        swr_free(&audioResampleContext);
        return false;
    }
    return true;                     
}

void FFWrapper::resampleAudio(const AVFrame* frame, uint8_t** dst_data, int dst_samples) {
    swr_convert(audioResampleContext, 
        dst_data, dst_samples, 
        (const uint8_t**)frame->extended_data, frame->nb_samples);
}

bool FFWrapper::open(const char* url) {
    // open input file, and allocate format context
    if (avformat_open_input(&formatContext, url, nullptr, nullptr) < 0) {
        LOGE("avformat_open_input(url=%s) failed", url);
        return false;
    }

    // retrieve stream information
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        LOGE("avformat_find_stream_info failed");
        return false;
    }

    // find video/audio stream
    videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (videoIndex < 0 || audioIndex < 0) {
        LOGE("av_find_best_stream failed: videoIndex=%d, audioIndex=%d", 
            videoIndex, audioIndex);
        return false;
    }

    if (videoIndex >= 0) {
        // find decoder for video stream
        AVCodec* videoDecoder = avcodec_find_decoder(formatContext->streams[videoIndex]->codecpar->codec_id);
        if (!videoDecoder) {
            LOGE("avcodec_find_decoder(videoIndex=%d) failed", videoIndex);
            return false;
        }

        // allocate a codec context for the decoder
        videoCodecContext = avcodec_alloc_context3(videoDecoder);
        if (!videoCodecContext) {
            LOGE("avcodec_alloc_context3(videoDecoder=%p) failed", videoDecoder);
            return false; 
        }

        // copy codec parameters from input stream to output codec context
        if (avcodec_parameters_to_context(videoCodecContext, formatContext->streams[videoIndex]->codecpar) < 0) {
            LOGE("avcodec_parameters_to_context(videoIndex=%d) failed", videoIndex);
            return false;
        }

        // Init the decoders with reference counting
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "refcounted_frames", "1", 0);
        if (avcodec_open2(videoCodecContext, videoDecoder, &opts) < 0) {
            LOGE("avcodec_open2(videoIndex=%d) failed", videoIndex);
            return false;
         }
    }

    if (audioIndex >= 0) {
        // find decoder for audio stream
        AVCodec* audioDecoder = avcodec_find_decoder(formatContext->streams[audioIndex]->codecpar->codec_id);
        if (!audioDecoder) {
            LOGE("avcodec_find_decoder(audioIndex=%d) failed", audioIndex);
            return false;
        }

        // allocate a codec context for the decoder
        audioCodecContext = avcodec_alloc_context3(audioDecoder);
        if (!audioCodecContext) {
            LOGE("avcodec_alloc_context3(audioDecoder=%p) failed", audioDecoder);
            return false; 
        }

        // copy codec parameters from input stream to output codec context
        if (avcodec_parameters_to_context(audioCodecContext, formatContext->streams[audioIndex]->codecpar) < 0) {
            LOGE("avcodec_parameters_to_context(audioIndex=%d) failed", audioIndex);
            return false;
        }

        // Init the decoders with reference counting
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "refcounted_frames", "1", 0);
        if (avcodec_open2(audioCodecContext, audioDecoder, &opts) < 0) {
            LOGE("avcodec_open2(audioIndex=%d) failed", audioIndex);
            return false;
         }
    }

    // dump status    
    LOGI("open %s succeed. start_time=%.6g, duration=%.6g", 
        url, double(formatContext->start_time)/AV_TIME_BASE, double(formatContext->duration)/AV_TIME_BASE);

    if (videoCodecContext) {
        LOGI("video_index=%d, pix_fmt=%s, video_size=%dx%d, start_time=%.6g, duration=%.6g, frames=%llu, fps=%.6g, refcounted=%d",
            videoIndex, av_get_pix_fmt_name(videoCodecContext->pix_fmt),
            videoCodecContext->width, videoCodecContext->height,
            av_q2d(formatContext->streams[videoIndex]->time_base)*formatContext->streams[videoIndex]->start_time,
            av_q2d(formatContext->streams[videoIndex]->time_base)*formatContext->streams[videoIndex]->duration,
            formatContext->streams[videoIndex]->nb_frames,
            av_q2d(formatContext->streams[videoIndex]->avg_frame_rate),
            videoCodecContext->refcounted_frames);
    }
    if (audioCodecContext) {
        LOGI("audio_index=%d, sample_fmt=%s, is_planar=%d, channels=%d, sample_rate=%d, start_time=%.6g, duration=%.6g, refcounted=%d",
            audioIndex, av_get_sample_fmt_name(audioCodecContext->sample_fmt), 
            av_sample_fmt_is_planar(audioCodecContext->sample_fmt),
            audioCodecContext->channels, audioCodecContext->sample_rate,
            av_q2d(formatContext->streams[audioIndex]->time_base)*formatContext->streams[audioIndex]->start_time,
            av_q2d(formatContext->streams[audioIndex]->time_base)*formatContext->streams[audioIndex]->duration,
            audioCodecContext->refcounted_frames);
    }
    return true;
}

void FFWrapper::close() {
    if (videoCodecContext) {
        avcodec_free_context(&videoCodecContext);
        videoIndex = -1;
    }
    if (audioCodecContext) {
        avcodec_free_context(&audioCodecContext);
        audioIndex = -1;
    }
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
}

bool FFWrapper::seek(int64_t timestamp) {
    // NOTES: timestamp is in AV_TIME_BASE units
    if (av_seek_frame(formatContext, -1, timestamp, 0) < 0) {
        LOGE("av_seek_frame(timestamp=%llu) failed", timestamp);
        return false;
    }
    return true;
}



