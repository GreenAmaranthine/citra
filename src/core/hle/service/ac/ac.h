// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Kernel {
class Event;
} // namespace Kernel

namespace Service::AC {

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        Interface(std::shared_ptr<Module> ac, const char* name);

        void CreateDefaultConfig(Kernel::HLERequestContext& ctx);
        void ConnectAsync(Kernel::HLERequestContext& ctx);
        void GetConnectResult(Kernel::HLERequestContext& ctx);
        void CloseAsync(Kernel::HLERequestContext& ctx);
        void GetCloseResult(Kernel::HLERequestContext& ctx);
        void GetWifiStatus(Kernel::HLERequestContext& ctx);
        void GetInfraPriority(Kernel::HLERequestContext& ctx);
        void SetRequestEulaVersion(Kernel::HLERequestContext& ctx);
        void RegisterDisconnectEvent(Kernel::HLERequestContext& ctx);
        void GetConnectingSsidLength(Kernel::HLERequestContext& ctx);
        void IsConnected(Kernel::HLERequestContext& ctx);
        void SetClientVersion(Kernel::HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> ac;
    };

protected:
    struct ACConfig {
        std::array<u8, 0x200> data;
    };

    ACConfig default_config{};

    bool ac_connected{};

    Kernel::SharedPtr<Kernel::Event> close_event;
    Kernel::SharedPtr<Kernel::Event> connect_event;
    Kernel::SharedPtr<Kernel::Event> disconnect_event;

    std::string connected_network_name{""};
};

void InstallInterfaces(Core::System& system);

} // namespace Service::AC
