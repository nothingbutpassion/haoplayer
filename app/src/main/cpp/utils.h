#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

template <typename T>
class Queue
{
public:
    Queue(size_t maxsize = 256) : maxSize(maxsize) {}

    bool empty() {
        std::unique_lock<std::mutex> lock(m);
        return q.empty();
    }

    int size() {
        std::unique_lock<std::mutex> lock(m);
        return q.size();
    }

    // NOTES: timeout unit is milliseconds
    bool push(const T& e, long timeout=0) {
        std::unique_lock<std::mutex> lock(m);
        if (q.size() == maxSize) {
            popCondition.wait_for(lock, std::chrono::milliseconds(timeout));
        } 
        if (q.size() == maxSize) {
            return false;
        } 
        q.push(e);
        pushCondition.notify_all();
        return true;
    }

    // NOTES: timeout unit is milliseconds
    bool pop(T& e, long timeout=0) {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) {
            pushCondition.wait_for(lock, std::chrono::milliseconds(timeout));
        }
        if (q.empty()) {
            return false;
        }
        e = q.front();
        q.pop();
        popCondition.notify_all();
        return true;
    }

private:
    std::queue<T> q;
    std::mutex m;
    std::condition_variable pushCondition;
    std::condition_variable popCondition;
    size_t maxSize;
};



template <typename T, typename Compare = std::greater<T>>
class PriorityQueue
{
public:
    PriorityQueue(size_t maxsize = 256, Compare compare = Compare()) 
        : q(compare), maxSize(maxsize) {}

    bool empty() {
        std::unique_lock<std::mutex> lock(m);
        return q.empty();
    }

    int size() {
        std::unique_lock<std::mutex> lock(m);
        return q.size();
    }

    // NOTES: timeout unit is milliseconds
    bool push(const T& e, long timeout=0) {
        std::unique_lock<std::mutex> lock(m);
        if (q.size() == maxSize) {
            popCondition.wait_for(lock, std::chrono::milliseconds(timeout));
        } 
        if (q.size() == maxSize) {
            return false;
        } 
        q.push(e);
        pushCondition.notify_all();
        return true;
    }

    // NOTES: timeout unit is milliseconds
    bool pop(T& e, long timeout=0) {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) {
            pushCondition.wait_for(lock, std::chrono::milliseconds(timeout));
        }
        if (q.empty()) {
            return false;
        }
        e = q.top();
        q.pop();
        popCondition.notify_all();
        return true;
    }

private:
    std::priority_queue<T, std::vector<T>, Compare> q; 
    size_t maxSize;
    std::mutex m;
    std::condition_variable pushCondition;
    std::condition_variable popCondition;
    
};