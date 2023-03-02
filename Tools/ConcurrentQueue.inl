#pragma once
#include "ConcurrentQueue.h"

namespace Tool {
    template<typename T>
    void ConcurrentQueue<T>::Push(T const& value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueue.push(value);
        mCv.notify_one();
    }

    template<typename T>
    void ConcurrentQueue<T>::Push(T&& value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueue.push(std::forward<T>(value));
        mCv.notify_one();
    }

    template<typename T>
    void ConcurrentQueue<T>::WaitPop(T& value) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCv.wait(lock, [this] {return !mQueue.empty(); });
        value = std::move(mQueue.front());
        mQueue.pop();
    }

    template<typename T>
    bool ConcurrentQueue<T>::TryPop(T& value) {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mQueue.empty()) return false;

        value = std::move(mQueue.front());
        mQueue.pop();
        return true;
    }

    template<typename T>
    bool ConcurrentQueue<T>::Empty() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mQueue.empty();
    }

    template<typename T>
    size_t ConcurrentQueue<T>::Size() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mQueue.size();
    }
}