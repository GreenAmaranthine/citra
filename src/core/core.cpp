// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <utility>
#include <enet/enet.h>
#include "audio_core/hle/hle.h"
#include "common/logging/log.h"
#include "core/cheats/cheats.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu/cpu.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/hle/service/fs/archive.h"
#include "core/hle/service/fs/fs_user.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"
#include "core/hw/hw.h"
#include "core/loader/loader.h"
#include "core/memory_setup.h"
#include "core/movie.h"
#include "network/room.h"
#include "network/room_member.h"
#ifdef ENABLE_SCRIPTING
#include "core/rpc/rpc_server.h"
#endif
#include "core/settings.h"
#include "video_core/renderer/renderer.h"
#include "video_core/video_core.h"

namespace Core {

System System::s_instance;

void System::Init1() {
    if (enet_initialize() != 0) {
        LOG_ERROR(Network, "Error when initializing ENet");
        return;
    }
    room = std::make_unique<Network::Room>();
    room_member = std::make_unique<Network::RoomMember>();
    movie = std::make_unique<Movie>();
}

System::~System() {
    if (room_member) {
        if (room_member->IsConnected())
            room_member->Leave();
        room_member.reset();
    }
    if (room) {
        if (room->IsOpen())
            room->Destroy();
        room.reset();
    }
    enet_deinitialize();
    movie.reset();
}

System::ResultStatus System::RunLoop() {
    status = ResultStatus::Success;
    if (!cpu_core)
        return ResultStatus::ErrorNotInitialized;
    if (!running.load(std::memory_order::memory_order_relaxed)) {
        std::unique_lock lock{running_mutex};
        running_cv.wait(lock);
    }
    if (!dsp_core->IsOutputAllowed()) {
        // Draw black screens to the emulator window
        VideoCore::g_renderer->SwapBuffers();
        // Sleep for one frame or the PC would overheat
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
        return ResultStatus::Success;
    }
    // If we don't have a currently active thread then don't execute instructions,
    // instead advance to the next event and try to yield to the next thread
    if (!kernel->GetThreadManager().GetCurrentThread()) {
        LOG_TRACE(Core_ARM11, "Idling");
        timing->Idle();
        timing->Advance();
        PrepareReschedule();
    } else {
        timing->Advance();
        cpu_core->Run();
    }
    HW::Update();
    Reschedule();
    if (shutdown_requested.exchange(false))
        return ResultStatus::ShutdownRequested;
    return status;
}

System::ResultStatus System::Load(Frontend& frontend, const std::string& filepath) {
    program_loader = Loader::GetLoader(*this, filepath);
    LOG_DEBUG(Frontend, "getloader");
    if (!program_loader) {
        LOG_ERROR(Core, "Failed to obtain loader for {}!", filepath);
        return ResultStatus::ErrorGetLoader;
    }
    LOG_DEBUG(Frontend, "loader valid");
    std::pair<std::optional<u32>, Loader::ResultStatus> system_mode{
        program_loader->LoadKernelSystemMode()};
    LOG_DEBUG(Frontend, "load system mode");
    if (system_mode.second != Loader::ResultStatus::Success) {
        LOG_ERROR(Core, "Failed to determine system mode (Error {})!",
                  static_cast<int>(system_mode.second));
        switch (system_mode.second) {
        case Loader::ResultStatus::ErrorEncrypted:
            return ResultStatus::ErrorLoader_ErrorEncrypted;
        case Loader::ResultStatus::ErrorInvalidFormat:
            return ResultStatus::ErrorLoader_ErrorInvalidFormat;
        default:
            return ResultStatus::ErrorSystemMode;
        }
    }
    ASSERT(system_mode.first);
    LOG_DEBUG(Frontend, "assert system mode ok");
    auto init_result{Init(frontend, *system_mode.first)};
    LOG_DEBUG(Frontend, "init ok");
    if (init_result != ResultStatus::Success) {
        LOG_ERROR(Core, "Failed to initialize system (Error {})!", static_cast<u32>(init_result));
        Shutdown();
        return init_result;
    }
    LOG_DEBUG(Frontend, "init success");
    Kernel::SharedPtr<Kernel::Process> process;
    LOG_DEBUG(Frontend, "process create");
    const auto load_result{program_loader->Load(process)};
    LOG_DEBUG(Frontend, "program load ok");
    kernel->SetCurrentProcess(process);
    LOG_DEBUG(Frontend, "Set current process ok");
    if (Loader::ResultStatus::Success != load_result) {
        LOG_ERROR(Core, "Failed to load file (Error {})!", static_cast<u32>(load_result));
        Shutdown();
        switch (load_result) {
        case Loader::ResultStatus::ErrorEncrypted:
            return ResultStatus::ErrorLoader_ErrorEncrypted;
        case Loader::ResultStatus::ErrorInvalidFormat:
            return ResultStatus::ErrorLoader_ErrorInvalidFormat;
        default:
            return ResultStatus::ErrorLoader;
        }
    }
    LOG_DEBUG(Frontend, "load success");
    Memory::SetCurrentPageTable(&kernel->GetCurrentProcess()->vm_manager.page_table);
    cheat_engine = std::make_unique<Cheats::CheatEngine>(*this);
    status = ResultStatus::Success;
    m_filepath = filepath;
    LOG_DEBUG(Frontend, "System::Load: {}", static_cast<u32>(status));
    return status;
}

void System::PrepareReschedule() {
    cpu_core->PrepareReschedule();
    reschedule_pending = true;
}

PerfStats::Results System::GetAndResetPerfStats() {
    return perf_stats.GetAndResetStats(timing->GetGlobalTimeUs());
}

void System::Reschedule() {
    if (!reschedule_pending)
        return;
    reschedule_pending = false;
    kernel->GetThreadManager().Reschedule();
}

System::ResultStatus System::Init(Frontend& frontend, u32 system_mode) {
    LOG_DEBUG(HW_Memory, "initialized OK");
    m_frontend = &frontend;
    LOG_DEBUG(Frontend, "Frontend init ok");
    timing = std::make_unique<Core::Timing>();
    LOG_DEBUG(Frontend, "timing init ok");
    kernel = std::make_unique<Kernel::KernelSystem>(*this);
    LOG_DEBUG(Frontend, "kernel init ok");
    // Initialize FS, CFG and memory
    service_manager = std::make_unique<Service::SM::ServiceManager>(*this);
    LOG_DEBUG(Frontend, "sm init ok");
    archive_manager = std::make_unique<Service::FS::ArchiveManager>(*this);
    LOG_DEBUG(Frontend, "archive manager init ok");
    Service::FS::InstallInterfaces(*this);
    LOG_DEBUG(Frontend, "fs init ok");
    Service::CFG::InstallInterfaces(*this);
    LOG_DEBUG(Frontend, "cfg init ok");
    kernel->MemoryInit(system_mode);
    LOG_DEBUG(Frontend, "memory init ok");
    cpu_core = std::make_unique<Cpu>(*this);
    LOG_DEBUG(Frontend, "cpu init ok");
    dsp_core = std::make_unique<AudioCore::DspHle>(*this);
    LOG_DEBUG(Frontend, "dsp init ok");
    dsp_core->EnableStretching(Settings::values.enable_audio_stretching);
    LOG_DEBUG(Frontend, "set stretching ok");
#ifdef ENABLE_SCRIPTING
    rpc_server = std::make_unique<RPC::RPCServer>(*this);
    LOG_DEBUG(Frontend, "rpc init ok");
#endif
    shutdown_requested = false;
    sleep_mode_enabled = false;
    LOG_DEBUG(Frontend, "atomics init ok");
    HW::Init();
    LOG_DEBUG(Frontend, "hw init ok");
    Service::Init(*this);
    LOG_DEBUG(Frontend, "services init ok");
    auto result{VideoCore::Init(*this)};
    LOG_DEBUG(Frontend, "video core init ok");
    if (result != ResultStatus::Success)
        return result;
    LOG_DEBUG(Core, "Initialized OK");
    // Reset counters and set time origin to current frame
    GetAndResetPerfStats();
    perf_stats.BeginSystemFrame();
    LOG_DEBUG(Frontend, "success");
    return ResultStatus::Success;
}

const Service::SM::ServiceManager& System::ServiceManager() const {
    return *service_manager;
}

Service::SM::ServiceManager& System::ServiceManager() {
    return *service_manager;
}

const Service::FS::ArchiveManager& System::ArchiveManager() const {
    return *archive_manager;
}

Service::FS::ArchiveManager& System::ArchiveManager() {
    return *archive_manager;
}

const Kernel::KernelSystem& System::Kernel() const {
    return *kernel;
}

Kernel::KernelSystem& System::Kernel() {
    return *kernel;
}

const Cheats::CheatEngine& System::CheatEngine() const {
    return *cheat_engine;
}

Cheats::CheatEngine& System::CheatEngine() {
    return *cheat_engine;
}

const Timing& System::CoreTiming() const {
    return *timing;
}

Timing& System::CoreTiming() {
    return *timing;
}

const Network::Room& System::Room() const {
    return *room;
}

Network::Room& System::Room() {
    return *room;
}

const Network::RoomMember& System::RoomMember() const {
    return *room_member;
}

Network::RoomMember& System::RoomMember() {
    return *room_member;
}

const Movie& System::MovieSystem() const {
    return *movie;
}

Movie& System::MovieSystem() {
    return *movie;
}

const Frontend& System::GetFrontend() const {
    return *m_frontend;
}

Frontend& System::GetFrontend() {
    return *m_frontend;
}

void System::Shutdown() {
    // Shutdown emulation session
    cpu_core.reset();
    cheat_engine.reset();
    VideoCore::Shutdown();
    kernel.reset();
    HW::Shutdown();
#ifdef ENABLE_SCRIPTING
    rpc_server.reset();
#endif
    service_manager.reset();
    dsp_core.reset();
    timing.reset();
    program_loader.reset();
    room_member->SendProgram(std::string{});
    LOG_DEBUG(Core, "Shutdown OK");
}

void System::Restart() {
    SetProgram(m_filepath);
}

void System::SetProgram(const std::string& path) {
    shutdown_requested = true;
    set_program_file_path = path;
}

void System::CloseProgram() {
    SetProgram("");
}

} // namespace Core
