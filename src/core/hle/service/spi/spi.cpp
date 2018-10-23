// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/service/spi/spi_cd2.h"
#include "core/hle/service/spi/spi_cs2.h"
#include "core/hle/service/spi/spi_cs3.h"
#include "core/hle/service/spi/spi_def.h"
#include "core/hle/service/spi/spi_nor.h"

namespace Service::SPI {

Module::Module() = default;
Module::~Module() = default;

Module::Interface::Interface(std::shared_ptr<Module> spi, const char* name)
    : ServiceFramework{name}, spi{std::move(spi)} {}

Module::Interface::~Interface() = default;

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    auto spi{std::make_shared<Module>()};
    std::make_shared<SPI_CD2>(spi)->InstallAsService(service_manager);
    std::make_shared<SPI_CS2>(spi)->InstallAsService(service_manager);
    std::make_shared<SPI_CS3>(spi)->InstallAsService(service_manager);
    std::make_shared<SPI_DEF>(spi)->InstallAsService(service_manager);
    std::make_shared<SPI_NOR>(spi)->InstallAsService(service_manager);
}

} // namespace Service::SPI
