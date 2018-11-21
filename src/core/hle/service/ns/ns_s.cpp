// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/ns/ns_s.h"

namespace Service::NS {

NS_S::NS_S(Core::System& system) : ServiceFramework{"ns:s", 2}, system{system} {
    static const FunctionInfo functions[]{
        {0x000100C0, nullptr, "LaunchFIRM"},
        {0x000200C0, &NS_S::LaunchTitle, "LaunchTitle"},
        {0x00030000, nullptr, "TerminateProgram"},
        {0x00040040, nullptr, "TerminateProcess"},
        {0x000500C0, nullptr, "LaunchProgramFIRM"},
        {0x00060042, nullptr, "SetFIRMParams4A0"},
        {0x00070042, nullptr, "CardUpdateInitialize"},
        {0x00080000, nullptr, "CardUpdateShutdown"},
        {0x000D0140, nullptr, "SetTWLBannerHMAC"},
        {0x000E0000, &NS_S::ShutdownAsync, "ShutdownAsync"},
        {0x00100180, nullptr, "RebootSystem"},
        {0x00110100, nullptr, "TerminateTitle"},
        {0x001200C0, nullptr, "SetProgramCpuTimeLimit"},
        {0x00150140, nullptr, "LaunchProgram"},
        {0x00160000, &NS_S::RebootSystemClean, "RebootSystemClean"},
    };
    RegisterHandlers(functions);
}

NS_S::~NS_S() = default;

} // namespace Service::NS
