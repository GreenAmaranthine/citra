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

void ReloadInputDevices() {
    auto& system{Core::System::GetInstance()};
    if (!system.IsPoweredOn()) {
        return;
    }

    auto& sm{system.ServiceManager()};

    auto ir_user{sm.GetService<IR_USER>("ir:USER")};
    auto ir_rst{sm.GetService<IR_RST>("ir:rst")};
    ir_user->ReloadInputDevices();
    ir_rst->ReloadInputDevices();
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<IR_U>()->InstallAsService(service_manager);
    std::make_shared<IR_USER>()->InstallAsService(service_manager);
    std::make_shared<IR_RST>()->InstallAsService(service_manager);
}

} // namespace Service::IR
