// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nim/nim_s.h"

namespace Service::NIM {

void NIM_S::CheckSysupdateAvailableSOAP(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0xA, 1, 0};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(0);
}

NIM_S::NIM_S() : ServiceFramework{"nim:s", 1} {
    const FunctionInfo functions[]{
        {0x000A0000, &NIM_S::CheckSysupdateAvailableSOAP, "CheckSysupdateAvailableSOAP"},
        {0x0016020A, nullptr, "ListTitles"},
        {0x00290000, nullptr, "AccountCheckBalanceSOAP"},
        {0x002D0042, nullptr, "DownloadTickets"},
        {0x00420240, nullptr, "StartDownload"},
    };
    RegisterHandlers(functions);
}

NIM_S::~NIM_S() = default;

} // namespace Service::NIM
