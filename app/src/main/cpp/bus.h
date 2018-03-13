                                                                                     #pragma once

#include "utils.h"

#define MESSAGE_ERROR_SOURCE        -1
#define MESSAGE_ERROR_FLUSH         -2
#define MESSAGE_ERROR_DECODE        -3
#define MESSAGE_ERROR_SEEK          -4
               
#define MESSAGE_EOS                  1
#define MESSAGE_SEEK_FINISHED        2
#define MESSAGE_FLUSH_FINISHED       3   

struct Message {
    Message(int id, void* data) : id(id), data(data) {}
    Message(int id) : id(id) {}
    Message() {}
    int id = -1;
    void* data = nullptr;
};

struct Bus {
    virtual bool sendMessage(const Message& messages) {
        return messageQueue.push(messages);
    }
    virtual bool getMessage(Message& message) {
        return messageQueue.pop(message);
    }
private:
    Queue<Message> messageQueue;
};