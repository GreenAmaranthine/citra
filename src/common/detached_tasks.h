// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <condition_variable>
#include <functional>

namespace Common {

/**
 * A background manager which ensures that all detached tasks are finished before program exits.
 *
 * To make detached task safe, a single DetachedTasks object should be placed in the main(), and
 * call WaitForAllTasks() after all program execution but before global/static variable destruction.
 * Any potentially unsafe detached task should be executed via DetachedTasks::AddTask.
 */
class DetachedTasks {
public:
    DetachedTasks();
    ~DetachedTasks();
    void WaitForAllTasks();

    static void AddTask(std::function<void()> task);

private:
    static DetachedTasks* instance;

    std::condition_variable cv;
    std::mutex mutex;
    int count{};
};

} // namespace Common
