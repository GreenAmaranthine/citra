// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/service/pdn/pdn.h"
#include "core/hle/service/pdn/pdn_c.h"
#include "core/hle/service/pdn/pdn_d.h"
#include "core/hle/service/pdn/pdn_g.h"
#include "core/hle/service/pdn/pdn_i.h"
#include "core/hle/service/pdn/pdn_s.h"

namespace Service::PDN {

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    std::make_shared<PDN_C>()->InstallAsService(service_manager);
    std::make_shared<PDN_D>()->InstallAsService(service_manager);
    std::make_shared<PDN_G>()->InstallAsService(service_manager);
    std::make_shared<PDN_I>()->InstallAsService(service_manager);
    std::make_shared<PDN_S>()->InstallAsService(service_manager);
}

} // namespace Service::PDN
