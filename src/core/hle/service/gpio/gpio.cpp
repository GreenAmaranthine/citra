// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/service/gpio/gpio_cdc.h"
#include "core/hle/service/gpio/gpio_hid.h"
#include "core/hle/service/gpio/gpio_ir.h"
#include "core/hle/service/gpio/gpio_mcu.h"
#include "core/hle/service/gpio/gpio_nfc.h"
#include "core/hle/service/gpio/gpio_nwm.h"
#include "core/hle/service/gpio/gpio_qtm.h"

namespace Service::GPIO {

Module::Module() = default;
Module::~Module() = default;

Module::Interface::Interface(std::shared_ptr<Module> gpio, const char* name)
    : ServiceFramework{name}, gpio{std::move(gpio)} {}

Module::Interface::~Interface() = default;

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    auto gpio{std::make_shared<Module>()};
    std::make_shared<GPIO_CDC>(gpio)->InstallAsService(service_manager);
    std::make_shared<GPIO_HID>(gpio)->InstallAsService(service_manager);
    std::make_shared<GPIO_IR>(gpio)->InstallAsService(service_manager);
    std::make_shared<GPIO_MCU>(gpio)->InstallAsService(service_manager);
    std::make_shared<GPIO_NFC>(gpio)->InstallAsService(service_manager);
    std::make_shared<GPIO_NWM>(gpio)->InstallAsService(service_manager);
    std::make_shared<GPIO_QTM>(gpio)->InstallAsService(service_manager);
}

} // namespace Service::GPIO
