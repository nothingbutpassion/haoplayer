#pragma once

#include <string>
#include <mutex>
#include <stdio.h>

enum State {
    STATE_NULL,
    STATE_READY,
    STATE_PAUSED,
    STATE_PLAYING
};

inline std::string toString(State s) {
    const char* states[] = {
        "STATE_NULL",
        "STATE_READY",
        "STATE_PAUSED",
        "STATE_PLAYING"
    };
    return states[s];
}

inline const char* cstr(State s) {
    const char* states[] = {
        "STATE_NULL",
        "STATE_READY",
        "STATE_PAUSED",
        "STATE_PLAYING"
    };
    return states[s];
}

struct States {
    void setCurrent(State s) {
        std::unique_lock<std::mutex> lock(m); 
        current = s;
    }
    State getCurrent()  {
        std::unique_lock<std::mutex> lock(m);
        return current; 
    }

    void setNext(State s) {
        std::unique_lock<std::mutex> lock(m); 
        next = s; 
    }
    State getNext() {
        std::unique_lock<std::mutex> lock(m); 
        return next; 
    }

    void setPending(State s) {  
        std::unique_lock<std::mutex> lock(m); 
        pending = s; 
    }
    State getPending(State s) {
        std::unique_lock<std::mutex> lock(m); 
        return pending; 
    }

    void set(State current, State next, State pending) {
        std::unique_lock<std::mutex> lock(m);
        this->current = current;
        this->next = next;
        this->pending = pending;
    }
    void get(State& current, State& next, State& pending) {
        std::unique_lock<std::mutex> lock(m);
        current = this->current;
        next = this->next;
        pending = this->pending;
    }
    
private:
    State current = STATE_NULL;
    State next = STATE_NULL;
    State pending = STATE_NULL;
    std::mutex m;
};


