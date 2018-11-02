// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/service/ir/ir.h"
#include "core/hle/service/ir/ir_rst.h"
#include "core/hle/service/ir/ir_u.h"
#include "core/hle/service/ir/ir_user.h"
#include "core/hle/service/service.h"

namespace Service::IR {

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    std::make_shared<IR_U>()->InstallAsService(service_manager);
    std::make_shared<IR_USER>(system)->InstallAsService(service_manager);
    std::make_shared<IR_RST>(system)->InstallAsService(service_manager);
}

} // namespace Service::IR
