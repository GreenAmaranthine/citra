// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/act/act.h"
#include "core/hle/service/act/act_u.h"

namespace Service::ACT {

ACT_U::ACT_U(std::shared_ptr<Module> act) : Module::Interface{std::move(act), "act:u"} {
    static const FunctionInfo functions[]{
        {0x00010084, &ACT_U::Initialize, "Initialize"},
        {0x00020040, &ACT_U::GetErrorCode, "GetErrorCode"},
        {0x000600C2, &ACT_U::GetAccountDataBlock, "GetAccountDataBlock"},
        {0x000B0042, nullptr, "AcquireEulaList"},
        {0x000D0040, nullptr, "GenerateUuid"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::ACT
