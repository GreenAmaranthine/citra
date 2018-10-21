// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/act/act.h"
#include "core/hle/service/act/act_a.h"
#include "core/hle/service/act/act_u.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/hle/service/fs/archive.h"

namespace Service::ACT {

void Module::Interface::Initialize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x0001, 2, 4};
    u32 version{rp.Pop<u32>()};
    u32 shared_memory_size{rp.Pop<u32>()};
    ASSERT(rp.Pop<u32>() == 0x20);
    rp.Skip(1, false);
    ASSERT(rp.Pop<u32>() == 0);
    u32 shared_memory{rp.Pop<u32>()};
    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);
    LOG_WARNING(Service_ACT,
                "(STUBBED) version=0x{:08X}, shared_memory_size=0x{:X}, shared_memory=0x{:08X}",
                version, shared_memory_size, shared_memory);
}

void Module::Interface::GetErrorCode(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x0002, 1, 0};
    u32 error_code{rp.Pop<u32>()};
    IPC::ResponseBuilder rb{rp.MakeBuilder(2, 0)};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(error_code); // TODO: convert
    LOG_WARNING(Service_ACT, "(STUBBED)");
}

void Module::Interface::GetAccountDataBlock(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x0006, 3, 2};
    u8 unk{rp.Pop<u8>()};
    u32 size{rp.Pop<u32>()};
    BlkID id{rp.PopEnum<BlkID>()};
    auto& buffer{rp.PopMappedBuffer()};
    switch (id) {
    case BlkID::NNID: {
        std::string nnid{Common::UTF16ToUTF8(CFG::GetCurrentModule()->GetUsername())};
        nnid.resize(0x11);
        nnid = Common::ReplaceAll(nnid, " ", "_");
        buffer.Write(nnid.c_str(), 0, nnid.length());
        break;
    }
    case BlkID::Unknown6: {
        u32 value{1};
        buffer.Write(&value, 0, sizeof(u32));
        break;
    }
    case BlkID::U16MiiName: {
        std::u16string username{CFG::GetCurrentModule()->GetUsername()};
        buffer.Write(username.c_str(), 0, username.length());
        break;
    }
    case BlkID::PrincipalID: {
        u32 principal_id{0xDEADBEEF};
        buffer.Write(&principal_id, 0, sizeof(u32));
        break;
    }
    case BlkID::CountryName: {
        u8 country_code{CFG::GetCurrentModule()->GetCountryCode()};
        u16 country_name{CFG::country_codes[country_code]};
        buffer.Write(&country_name, 0, sizeof(u16));
        break;
    }
    case BlkID::Age: {
        u16 age{};
        buffer.Write(&age, 0, sizeof(u16));
        break;
    }
    case BlkID::Birthday: {
        Birthday birthday{};
        buffer.Write(&birthday, 0, sizeof(Birthday));
        break;
    }
    case BlkID::InfoStruct: {
        InfoBlock info{};
        std::u16string username{CFG::GetCurrentModule()->GetUsername()};
        username.copy(info.MachinUserName, username.length());
        buffer.Write(&info, 0, username.length());
        break;
    }
    default: {
        UNIMPLEMENTED();
        break;
    }
    }
    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 2)};
    rb.Push(RESULT_SUCCESS);
    rb.PushMappedBuffer(buffer);
    LOG_WARNING(Service_ACT, "(STUBBED) unk=0x{:02X}, size=0x{:X}, id=0x{:X}", unk, size,
                static_cast<u32>(id));
}

Module::Interface::Interface(std::shared_ptr<Module> act, const char* name)
    : ServiceFramework{name}, act{std::move(act)} {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto act{std::make_shared<Module>()};
    std::make_shared<ACT_A>(act)->InstallAsService(service_manager);
    std::make_shared<ACT_U>(act)->InstallAsService(service_manager);
}

} // namespace Service::ACT
