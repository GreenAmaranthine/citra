// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Service::LDR {

struct ClientSlot : public Kernel::SessionRequestHandler::SessionDataBase {
    VAddr loaded_crs{}; ///< The virtual address of the static module
};

class RO final : public ServiceFramework<RO, ClientSlot> {
public:
    explicit RO(Core::System& system);

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void LoadCRR(Kernel::HLERequestContext& ctx);
    void UnloadCRR(Kernel::HLERequestContext& ctx);
    void LoadCRO(Kernel::HLERequestContext& ctx, bool link_on_load_bug_fix);

    template <bool link_on_load_bug_fix>
    void LoadCRO(Kernel::HLERequestContext& ctx) {
        LoadCRO(ctx, link_on_load_bug_fix);
    }

    void UnloadCRO(Kernel::HLERequestContext& ctx);
    void LinkCRO(Kernel::HLERequestContext& ctx);
    void UnlinkCRO(Kernel::HLERequestContext& ctx);
    void Shutdown(Kernel::HLERequestContext& ctx);

    Core::System& system;
};

void InstallInterfaces(Core::System& system);

} // namespace Service::LDR
