// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/service/cdc/cdc_chk.h"
#include "core/hle/service/cdc/cdc_csn.h"
#include "core/hle/service/cdc/cdc_dsp.h"
#include "core/hle/service/cdc/cdc_hid.h"
#include "core/hle/service/cdc/cdc_lgy.h"
#include "core/hle/service/cdc/cdc_mic.h"

namespace Service::CDC {

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    std::make_shared<CDC_CHK>()->InstallAsService(service_manager);
    std::make_shared<CDC_CSN>()->InstallAsService(service_manager);
    std::make_shared<CDC_DSP>()->InstallAsService(service_manager);
    std::make_shared<CDC_HID>()->InstallAsService(service_manager);
    std::make_shared<CDC_LGY>()->InstallAsService(service_manager);
    std::make_shared<CDC_MIC>()->InstallAsService(service_manager);
}

} // namespace Service::CDC
