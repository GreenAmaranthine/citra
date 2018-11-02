// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "common/common_types.h"

namespace Common {

class Event {
public:
    Event() : is_set(false) {}

    void Set() {
        std::lock_guard lk{mutex};
        if (!is_set) {
            is_set = true;
            condvar.notify_one();
        }
    }

    void Wait() {
        std::unique_lock lk{mutex};
        condvar.wait(lk, [&] { return is_set; });
        is_set = false;
    }

    template <class Duration>
    bool WaitFor(const std::chrono::duration<Duration>& time) {
        std::unique_lock lk{mutex};
        if (!condvar.wait_for(lk, time, [this] { return is_set; }))
            return false;
        is_set = false;
        return true;
    }

    template <class Clock, class Duration>
    bool WaitUntil(const std::chrono::time_point<Clock, Duration>& time) {
        std::unique_lock lk{mutex};
        if (!condvar.wait_until(lk, time, [this] { return is_set; }))
            return false;
        is_set = false;
        return true;
    }

    void Reset() {
        std::unique_lock lk{mutex};
        // no other action required, since wait loops on the predicate and any lingering signal will
        // get cleared on the first iteration
        is_set = false;
    }

private:
    bool is_set;
    std::condition_variable condvar;
    std::mutex mutex;
};

} // namespace Common
