#include "log.h"
#include "bus.h"
#include "player.h"

#undef  LOG_TAG
#define LOG_TAG "player"

Player::Player() {
    bus = new Bus();
    clock = audioRender.getClock();

    for (Element* element : elememts) {
        element->setClock(clock);
        element->setBus(bus);
    }

    demuxer.setEngine(&ffWrapper);
    demuxer.setAudioSink(&audioDecoder);
    demuxer.setVideoSink(&videoDecoder);

    videoDecoder.setEngine(&ffWrapper);
    videoDecoder.setSource(&demuxer);
    videoDecoder.setVideoSink(&videoRender);
    videoRender.setEngine(&ffWrapper);
    videoRender.setSource(&videoDecoder);

    audioDecoder.setEngine(&ffWrapper);
    audioDecoder.setSource(&demuxer);
    audioDecoder.setAudioSink(&audioRender);
    audioRender.setEngine(&ffWrapper);
    audioRender.setSource(&audioDecoder);
}

Player::~Player() {
    delete bus;
    elememts.clear();
}

void Player::setDataSource(const char* url) {
    if (!validStates()) {
        return;
    }
    State s = elememts[0]->getState();
    if (s != STATE_NULL) {
        return;
    }
    demuxer.setSource(url);
}

void Player::setSurface(jobject surface) {
    videoRender.setSurface(surface);
}

void Player::play() {
    if (!validStates()) {
        return;
    }
    State ss[] = {STATE_NULL, STATE_READY, STATE_PAUSED, STATE_PLAYING};
    State i = elememts[0]->getState();
    while (i < STATE_PLAYING) {
        for (Element* e : elememts) {
            e->setState(ss[i+1]);
            if (e->getState() != ss[i+1]) {
                return;
            }
        }
        i = ss[i+1];
    }
}

void Player::stop() {
    if (!validStates()) {
        return;
    }
    State ss[] = {STATE_NULL, STATE_READY, STATE_PAUSED, STATE_PLAYING};
    State i = elememts[0]->getState();
    while (i > STATE_NULL) {
        for (Element* e : elememts) {
            e->setState(ss[i-1]);
            if (e->getState() != ss[i-1]) {
                return;
            }
        }
        i = ss[i-1];
    }
}

void Player::pause() {
    if (!validStates()) {
        return;
    }
    State ss[] = {STATE_NULL, STATE_READY, STATE_PAUSED, STATE_PLAYING};
    State i = elememts[0]->getState();
    while (i > STATE_PAUSED) {
        for (Element* e : elememts) {
            e->setState(ss[i-1]);
            if (e->getState() != ss[i-1]) {
                return;
            }
        }
        i = ss[i-1];
    }
    while (i < STATE_PAUSED) {
        for (Element* e : elememts) {
            e->setState(ss[i+1]);
            if (e->getState() != ss[i+1]) {
                return;
            }
        }
        i = ss[i+1];
    }
}

void Player::seek(int position) {
    if (!validStates()) {
        return;
    }
    State ss[] = {STATE_NULL, STATE_READY, STATE_PAUSED, STATE_PLAYING};
    State i = elememts[0]->getState();
    while (i > STATE_READY) {
        for (Element* e : elememts) {
            e->setState(ss[i-1]);
            if (e->getState() != ss[i-1]) {
                return;
            }
        }
        i = ss[i-1];
    }
    while (i < STATE_READY) {
        for (Element* e : elememts) {
            e->setState(ss[i+1]);
            if (e->getState() != ss[i+1]) {
                return;
            }
        }
        i = ss[i+1];
    }
    demuxer.seek(position);
}

int Player::getDuration() {
    if (demuxer.getState() >= STATE_READY) {
        return demuxer.getDuration();
    }
    return 0;
}

int Player::getPosition() {
    if (!validStates()) {
        return 0;
    }
    State s = elememts[0]->getState();
    if (s == STATE_NULL) {
        return 0;
    }
    return clock->runningTime()/1000;
}

bool Player::validStates() {
    State s = elememts[0]->getState();
    for (int i=1; i < elememts.size(); i++) {
        if (s != elememts[i]->getState()) {
            return false;
        }
    }
    return true;
}
