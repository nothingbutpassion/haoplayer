#pragma once

#include <stdint.h>
#include <chrono>
#include <atomic>

struct Clock {
    virtual int64_t runningTime() {
        return absoluteTime() - base.load();
    }

    virtual int64_t baseTime() {
        return base.load();
    }

    virtual int64_t absoluteTime() {
        using namespace std::chrono;
        system_clock::time_point tp = system_clock::now();
        system_clock::duration d = tp.time_since_epoch();
        return duration_cast<microseconds>(d).count();
    }

    // NOTES: base unit is microseconds
    virtual void setBaseTime(uint64_t baseTime) {
        base.store(baseTime);
    }

protected:
    std::atomic<int64_t> base;
};