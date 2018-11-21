// Copyrigh { 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/ns/ns.h"
#include "core/hle/service/ns/ns_s.h"
#include "core/loader/loader.h"
#include "core/settings.h"

namespace Service::NS {

Kernel::SharedPtr<Kernel::Process> Launch(Core::System& system, FS::MediaType media_type,
                                          u64 program_id) {
    auto path{AM::GetProgramContentPath(media_type, program_id)};
    auto loader{Loader::GetLoader(system, path)};
    if (!loader) {
        LOG_WARNING(Service_NS, "Couldn't find .app for program 0x{:016X}", program_id);
        return nullptr;
    }
    Kernel::SharedPtr<Kernel::Process> process;
    auto result{loader->Load(process)};
    if (result != Loader::ResultStatus::Success) {
        LOG_WARNING(Service_NS, "Error loading .app for program 0x{:016X}", program_id);
        return nullptr;
    }
    return process;
}

void NS_S::LaunchTitle(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x2, 3, 0};
    u64 program_id{rp.Pop<u64>()};
    u32 flags{rp.Pop<u32>()};
    FS::MediaType media_type{(program_id == 0) ? FS::MediaType::GameCard
                                               : AM::GetProgramMediaType(program_id)};
    if (!Settings::values.enable_ns_launch) {
        IPC::ResponseBuilder rb{rp.MakeBuilder(2, 0)};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(0);
    } else {
        auto process{Launch(system, media_type, program_id)};
        IPC::ResponseBuilder rb{rp.MakeBuilder(2, 0)};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(process ? process->process_id : 0);
    }
    LOG_DEBUG(Service_NS, "program_id={}, media_type={}, flags={}", program_id,
              static_cast<u32>(media_type), flags);
}

void NS_S::ShutdownAsync(Kernel::HLERequestContext& ctx) {
    system.CloseProgram();
    IPC::ResponseBuilder rb{ctx, 0xE, 1, 0};
    rb.Push(RESULT_SUCCESS);
}

void NS_S::RebootSystemClean(Kernel::HLERequestContext& ctx) {
    system.Restart();
    IPC::ResponseBuilder rb{ctx, 0x16, 1, 0};
    rb.Push(RESULT_SUCCESS);
}

void InstallInterfaces(Core::System& system) {
    std::make_shared<NS_S>(system)->InstallAsService(system.ServiceManager());
}

} // namespace Service::NS
