// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/service/i2c/i2c_cam.h"
#include "core/hle/service/i2c/i2c_deb.h"
#include "core/hle/service/i2c/i2c_eep.h"
#include "core/hle/service/i2c/i2c_hid.h"
#include "core/hle/service/i2c/i2c_ir.h"
#include "core/hle/service/i2c/i2c_lcd.h"
#include "core/hle/service/i2c/i2c_mcu.h"
#include "core/hle/service/i2c/i2c_nfc.h"
#include "core/hle/service/i2c/i2c_qtm.h"

namespace Service::I2C {

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    std::make_shared<I2C_CAM>()->InstallAsService(service_manager);
    std::make_shared<I2C_DEB>()->InstallAsService(service_manager);
    std::make_shared<I2C_EEP>()->InstallAsService(service_manager);
    std::make_shared<I2C_HID>()->InstallAsService(service_manager);
    std::make_shared<I2C_IR>()->InstallAsService(service_manager);
    std::make_shared<I2C_LCD>()->InstallAsService(service_manager);
    std::make_shared<I2C_MCU>()->InstallAsService(service_manager);
    std::make_shared<I2C_NFC>()->InstallAsService(service_manager);
    std::make_shared<I2C_QTM>()->InstallAsService(service_manager);
}

} // namespace Service::I2C
