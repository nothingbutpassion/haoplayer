#pragma once

#include <string>

#define VIDEO_ENGIN     0x01
#define VIDEO_SURFACE   0x02

struct VideoDevice {
    static VideoDevice* create(const std::string name);
    static void release(VideoDevice* device);
    virtual ~VideoDevice() {}
    virtual bool setProperty(int key, void* value) = 0;
    virtual int  getPixelFormat() = 0;
    virtual int  getWidth() = 0;
    virtual int  getHeight() = 0;
    virtual int  write(void* buf, int buflen) = 0;
};