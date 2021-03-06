// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "core/hle/service/service.h"

namespace Core {
struct TimingEventType;
} // namespace Core

namespace Kernel {
class SharedMemory;
} // namespace Kernel

namespace Service::IR {

class BufferManager;
class ExtraHID;

/// An interface representing a device that can communicate with console via ir:USER service
class IRDevice {
public:
    /**
     * A function object that implements the method to send data to the console, which takes a
     * vector of data to send.
     */
    using SendFunc = std::function<void(const std::vector<u8>& data)>;

    explicit IRDevice(Core::System& system, SendFunc send_func);
    virtual ~IRDevice();

    /// Called when connected with console
    virtual void OnConnect() = 0;

    /// Called when disconnected from console
    virtual void OnDisconnect() = 0;

    /// Called when data is received from the console. This is invoked by the ir:USER send function.
    virtual void OnReceive(const std::vector<u8>& data) = 0;

protected:
    /// Sends data to the console. The actual sending method is specified in the constructor
    void Send(const std::vector<u8>& data);

    Core::System& system;

private:
    const SendFunc send_func;
};

/// Interface to "ir:USER" service
class IR_USER final : public ServiceFramework<IR_USER> {
public:
    explicit IR_USER(Core::System& system);
    ~IR_USER();

    void ReloadInputDevices();

private:
    void InitializeIrNopShared(Kernel::HLERequestContext& ctx);
    void RequireConnection(Kernel::HLERequestContext& ctx);
    void GetReceiveEvent(Kernel::HLERequestContext& ctx);
    void GetSendEvent(Kernel::HLERequestContext& ctx);
    void Disconnect(Kernel::HLERequestContext& ctx);
    void GetConnectionStatusEvent(Kernel::HLERequestContext& ctx);
    void FinalizeIrNop(Kernel::HLERequestContext& ctx);
    void SendIrNop(Kernel::HLERequestContext& ctx);
    void ReleaseReceivedData(Kernel::HLERequestContext& ctx);

    void PutToReceive(const std::vector<u8>& payload);

    Kernel::SharedPtr<Kernel::Event> conn_status_event, send_event, receive_event;
    Kernel::SharedPtr<Kernel::SharedMemory> shared_memory;
    IRDevice* connected_device{nullptr};
    std::unique_ptr<BufferManager> receive_buffer;
    std::unique_ptr<ExtraHID> extra_hid;
};

} // namespace Service::IR
