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
            case VIDEO_WIDTH:
                width = *static_cast<int*>(value);
                break;
            case VIDEO_HEIGHT:
                height = *static_cast<int*>(value);
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
                // set video scale context
                if (width != buffer.width || height != buffer.height || format != buffer.format) {
                    ffWrapper->setVideoScale(frame, buffer.width, buffer.height, AV_PIX_FMT_RGBA);
                    width = buffer.width;
                    height = buffer.height;
                    format = buffer.format;
                }
                // scale video
                int dst_linesize = buffer.stride * 4;
                uint8_t* dst_data = static_cast<uint8_t*>(buffer.bits);

                using namespace std::chrono;
                system_clock::time_point tp = system_clock::now();
                ffWrapper->scaleVideo(frame, &dst_data, &dst_linesize);
                system_clock::duration d = system_clock::now() - tp;
                LOGT("scaleVideo duration is %lldms", duration_cast<milliseconds>(d).count());
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
