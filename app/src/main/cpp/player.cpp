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
    demuxer.setSource(url);
}

void Player::setSurface(jobject surface) {
    videoRender.setSurface(surface);
}

void Player::play() {
    if (!validStates()) {
        return;
    }
    State ss[] = {IDLE, READY, PAUSED, PLAYING};
    State i = elememts[0]->getState();
    while (i < PLAYING) {
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
    State ss[] = {IDLE, READY, PAUSED, PLAYING};
    State i = elememts[0]->getState();
    while (i > IDLE) {
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

}

void Player::seek(int position) {

}

int Player::getDuration() {
    return 0;
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
