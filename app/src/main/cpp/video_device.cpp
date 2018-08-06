#include <chrono>
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include "log.h"
#include "ffwrapper.h"
#include "video_device.h"


#undef  LOG_TAG 
#define LOG_TAG "VideoDevice"

extern JNIEnv* getJNIEnv(void);

class SurfaceDevice: public VideoDevice {
public:
    SurfaceDevice() {}
    ~SurfaceDevice() {}

    bool setProperty(int key, void* value) override {
        switch (key) {
            case VIDEO_ENGIN:
                ffWrapper = static_cast<FFWrapper*>(value);
                break;
            case VIDEO_SURFACE:
                if (window) {
                    ANativeWindow_release(window);
                    window = nullptr;
                }
                if (value) {
                    JNIEnv* env = getJNIEnv();
                    surface = static_cast<jobject>(value);
                    window = ANativeWindow_fromSurface(env, surface);
                }
                break;
            default:
                LOGE("setProperty: unsupported key=%d", key);
                return false;
        }
        return true;
    }

    int getPixelFormat() override {
        return AV_PIX_FMT_RGBA;
    }

    int getWidth() override {
        if (window) {
            return ANativeWindow_getWidth(window);
        }
        return 0;
    }

    int getHeight() override {
        if (window) {
            return ANativeWindow_getHeight(window);
        }
        return 0;
    }

    int write(void* buf, int buflen) override {
        int writed = 0;
        AVFrame* frame = static_cast<AVFrame*>(buf);
        ANativeWindow_Buffer buffer = {0};
        if (ANativeWindow_lock(window, &buffer, 0) == 0) {
            LOGD("write: window: width=%d, height=%d, stride=%d, format=%d",
                 buffer.width, buffer.height, buffer.stride, buffer.format);
            if (buffer.width > 0 && buffer.height > 0) {
                // adjust display width/height based on video width/height
                int W = buffer.width;
                int H = buffer.height;
                int offset = 0;
                if (buffer.width*frame->height > frame->width*buffer.height) {
                    W = buffer.height*frame->width/frame->height;
                    offset = (buffer.width - W)*4/2;
                } else {
                    H = buffer.width*frame->height/frame->width;
                    //offset = (buffer.height - H)*buffer.stride*4/2;
                }

                // set video scale context
                if (width != buffer.width || height != buffer.height || format != buffer.format) {
                    //ffWrapper->setVideoScale(frame, buffer.width, buffer.height, AV_PIX_FMT_RGBA);
                    ffWrapper->setVideoScale(frame, W, H, AV_PIX_FMT_RGBA);
                    width = buffer.width;
                    height = buffer.height;
                    format = buffer.format;
                }
                // scale video
                int dst_linesize = buffer.stride * 4;
                uint8_t* dst_data = static_cast<uint8_t*>(buffer.bits) + offset;

                using namespace std::chrono;
                system_clock::time_point tp = system_clock::now();
                ffWrapper->scaleVideo(frame, &dst_data, &dst_linesize);

                if (H < buffer.height) {
                    int h = (buffer.height - H)/2;
                    dst_data += h*buffer.stride*4;
                    ffWrapper->scaleVideo(frame, &dst_data, &dst_linesize);
                    dst_data += h*buffer.stride*4;
                    ffWrapper->scaleVideo(frame, &dst_data, &dst_linesize);

                    for (int j=0; j < buffer.height; j++) {
                        dst_data = static_cast<uint8_t*>(buffer.bits) + j*buffer.stride*4;
                        for (int i=0; i < buffer.width; i++) {
                            if (j < h) {
                                if (i < buffer.width/2) {
                                    dst_data[4*i+1] = 0;
                                    dst_data[4*i+2] = 0;
                                } else {
                                    dst_data[4*i] = 0;
                                }
                            } else if (j < 2*h) {
                                if (i < buffer.width/2) {
                                    dst_data[4*i] = 0;
                                    dst_data[4*i+2] = 0;
                                } else {
                                    dst_data[4*i+1] = 0;
                                }

                            } else {
                                if (i < buffer.width/2) {
                                    dst_data[4*i] = 0;
                                    dst_data[4*i+1] = 0;
                                } else {
                                    dst_data[4*i+2] = 0;
                                }
                            }
                        }
                    }

                }

                system_clock::duration d = system_clock::now() - tp;
                LOGD("scaleVideo duration is %lldms", duration_cast<milliseconds>(d).count());
                writed = dst_linesize * frame->height;
            }
            ANativeWindow_unlockAndPost(window);
        } else {
            // NOTES: window may be invalid. try to got a reference from surface.
            ANativeWindow_release(window);
            JNIEnv* env = getJNIEnv();
            window = ANativeWindow_fromSurface(env, surface);
        }
        ffWrapper->freeFrame(frame);
        return writed;
    }
private:
    int width = 0;
    int height = 0;
    int format = AV_PIX_FMT_NONE;
    FFWrapper* ffWrapper = nullptr;
    ANativeWindow* window = nullptr;
    jobject surface = nullptr;

};

VideoDevice* VideoDevice::create(const std::string name) {
    if (name != "SurfaceDevice") {
        return nullptr; 
    } 
    return new SurfaceDevice;
}

void VideoDevice::release(VideoDevice* device) {
    delete device;
}
