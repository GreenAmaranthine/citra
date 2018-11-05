// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <thread>
#include "common/assert.h"
#include "common/detached_tasks.h"

namespace Common {

DetachedTasks* DetachedTasks::instance{};

DetachedTasks::DetachedTasks() {
    ASSERT(!instance);
    instance = this;
}

DetachedTasks::~DetachedTasks() {
    LOG_INFO(Common, "Waiting for all detached tasks to end...");
    std::unique_lock lock{mutex};
    cv.wait(lock, [this]() { return count == 0; });
    instance = nullptr;
}

void DetachedTasks::AddTask(std::function<void()> task) {
    std::unique_lock lock{instance->mutex};
    ++instance->count;
    std::thread([task{std::move(task)}]() {
        task();
        std::unique_lock lock{instance->mutex};
        --instance->count;
        std::notify_all_at_thread_exit(instance->cv, std::move(lock));
    })
        .detach();
}

} // namespace Common
