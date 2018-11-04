// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/config_mem.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/kernel/shared_page.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/timer.h"

namespace Kernel {

KernelSystem::KernelSystem(Core::System& system) : system{system} {
    resource_limits = std::make_unique<ResourceLimitList>(*this);
    auto& timing{system.CoreTiming()};
    thread_manager = std::make_unique<ThreadManager>(system);
    timer_manager = std::make_unique<TimerManager>(timing);
}

KernelSystem::~KernelSystem() = default;

void KernelSystem::InitializeMemory(u32 system_mode) {
    MemoryInit(system_mode);
}

ResourceLimitList& KernelSystem::ResourceLimit() {
    return *resource_limits;
}

const ResourceLimitList& KernelSystem::ResourceLimit() const {
    return *resource_limits;
}

u32 KernelSystem::GenerateObjectID() {
    return next_object_id++;
}

SharedPtr<Process> KernelSystem::GetCurrentProcess() const {
    return current_process;
}

void KernelSystem::SetCurrentProcess(SharedPtr<Process> process) {
    current_process = std::move(process);
}

const ThreadManager& KernelSystem::GetThreadManager() const {
    return *thread_manager;
}

ThreadManager& KernelSystem::GetThreadManager() {
    return *thread_manager;
}

const TimerManager& KernelSystem::GetTimerManager() const {
    return *timer_manager;
}

TimerManager& KernelSystem::GetTimerManager() {
    return *timer_manager;
}

const SharedPage::Handler& KernelSystem::GetSharedPageHandler() const {
    return *shared_page_handler;
}

SharedPage::Handler& KernelSystem::GetSharedPageHandler() {
    return *shared_page_handler;
}

const ConfigMem::Handler& KernelSystem::GetConfigMemHandler() const {
    return *config_mem_handler;
}

ConfigMem::Handler& KernelSystem::GetConfigMemHandler() {
    return *config_mem_handler;
}

} // namespace Kernel
