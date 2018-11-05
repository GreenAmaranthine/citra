// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <condition_variable>
#include <functional>

namespace Common {

/// A background manager which ensures that all detached tasks are finished before program exits.
class DetachedTasks {
public:
    DetachedTasks();
    ~DetachedTasks();

    static void AddTask(std::function<void()> task);

private:
    static DetachedTasks* instance;

    std::condition_variable cv;
    std::mutex mutex;
    int count{};
};

} // namespace Common
