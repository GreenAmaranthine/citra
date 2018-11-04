// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/cpu/cpu.h"
#include "core/hle/service/hid/hid.h"
#include "core/memory.h"
#include "core/rpc/packet.h"
#include "core/rpc/rpc_server.h"

namespace RPC {

RPCServer::RPCServer() : server{*this} {
    LOG_INFO(RPC, "Starting RPC server ...");

    Start();

    LOG_INFO(RPC, "RPC started.");
}

RPCServer::~RPCServer() {
    LOG_INFO(RPC, "Stopping RPC ...");

    Stop();

    LOG_INFO(RPC, "RPC stopped.");
}

void RPCServer::HandleReadMemory(Packet& packet, u32 address, u32 data_size) {
    // Note: Memory read occurs asynchronously from the state of the emulator
    Memory::ReadBlock(address, packet.GetPacketData().data(), data_size);
    packet.SetPacketDataSize(data_size);
    packet.SendReply();
}

void RPCServer::HandleWriteMemory(Packet& packet, u32 address, const u8* data, u32 data_size) {
    // Note: Memory write occurs asynchronously from the state of the emulator
    Memory::WriteBlock(address, data, data_size);
    // If the memory happens to be executable code, make sure the changes become visible
    system.CPU().InvalidateCacheRange(address, data_size);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandlePadState(Packet& packet, u32 raw) {
    Service::HID::SetPadState(raw);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleTouchState(Packet& packet, s16 x, s16 y, bool valid) {
    Service::HID::SetTouchState(x, y, valid);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleMotionState(Packet& packet, s16 x, s16 y, s16 z, s16 roll, s16 pitch,
                                  s16 yaw) {
    Service::HID::SetMotionState(x, y, z, roll, pitch, yaw);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleCircleState(Packet& packet, s16 x, s16 y) {
    Service::HID::SetCircleState(x, y);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleSetResolution(Packet& packet, u16 resolution) {
    Settings::values.resolution_factor = resolution;
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleSetApplication(Packet& packet, const std::string& path) {
    Core::System::GetInstance().SetApplication(path);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleSetOverrideControls(Packet& packet, bool pad, bool touch, bool motion,
                                          bool circle) {
    Service::HID::SetOverrideControls(pad, touch, motion, circle);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandlePause(Packet& packet) {
    Core::System::GetInstance().SetRunning(false);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleResume(Packet& packet) {
    Core::System::GetInstance().SetRunning(true);
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleRestart(Packet& packet) {
    Core::System::GetInstance().Restart();
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleSetSpeedLimit(Packet& packet, u16 speed_limit) {
    Settings::values.use_frame_limit = true;
    Settings::values.frame_limit = speed_limit;
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleSetBackgroundColor(Packet& packet, float r, float g, float b) {
    Settings::values.bg_red = r;
    Settings::values.bg_green = g;
    Settings::values.bg_blue = b;
    Settings::Apply();
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleSetScreenRefreshRate(Packet& packet, int rate) {
    Settings::values.screen_refresh_rate = rate;
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleIsButtonPressed(Packet& packet, int button) {
    packet.SetPacketDataSize(sizeof(bool));
    packet.GetPacketData()[0] = (Service::HID::GetInputsThisFrame().hex & button) != 0;
    packet.SendReply();
}

void RPCServer::HandleSetFrameAdvancing(Packet& packet, bool enable) {
    Core::System::GetInstance().frame_limiter.SetFrameAdvancing(enable);
    if (cb_update_frame_advancing) {
        cb_update_frame_advancing();
    }
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

void RPCServer::HandleAdvanceFrame(Packet& packet) {
    Core::System::GetInstance().frame_limiter.AdvanceFrame();
    if (cb_update_frame_advancing) {
        cb_update_frame_advancing();
    }
    packet.SetPacketDataSize(0);
    packet.SendReply();
}

bool RPCServer::ValidatePacket(const PacketHeader& packet_header) {
    if (packet_header.version <= CURRENT_VERSION) {
        switch (packet_header.packet_type) {
        case PacketType::ReadMemory:
        case PacketType::WriteMemory:
        case PacketType::PadState:
        case PacketType::TouchState:
        case PacketType::MotionState:
        case PacketType::CircleState:
        case PacketType::SetResolution:
        case PacketType::SetApplication:
        case PacketType::SetOverrideControls:
        case PacketType::Pause:
        case PacketType::Resume:
        case PacketType::Restart:
        case PacketType::SetSpeedLimit:
        case PacketType::SetBackgroundColor:
        case PacketType::SetScreenRefreshRate:
        case PacketType::IsButtonPressed:
        case PacketType::SetFrameAdvancing:
        case PacketType::AdvanceFrame:
            if (packet_header.packet_size >= (sizeof(u32) * 2)) {
                return true;
            }
            break;
        default:
            break;
        }
    }
    return false;
}

void RPCServer::HandleSingleRequest(std::unique_ptr<Packet> request_packet) {
    bool success{};

    if (ValidatePacket(request_packet->GetHeader())) {
        // Currently, all request types use the address/data_size wire format
        u32 address;
        u32 data_size;
        std::memcpy(&address, request_packet->GetPacketData().data(), sizeof(address));
        std::memcpy(&data_size, request_packet->GetPacketData().data() + sizeof(address),
                    sizeof(data_size));
        switch (request_packet->GetPacketType()) {
        case PacketType::ReadMemory: {
            if (data_size > 0 && data_size <= MAX_READ_WRITE_SIZE) {
                HandleReadMemory(*request_packet, address, data_size);
                success = true;
            }
            break;
        }
        case PacketType::WriteMemory: {
            if (data_size > 0 && data_size <= MAX_READ_WRITE_SIZE) {
                const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
                HandleWriteMemory(*request_packet, address, data, data_size);
                success = true;
            }
            break;
        }
        case PacketType::PadState: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            u32 raw;
            std::memcpy(&raw, data, sizeof(u32));
            HandlePadState(*request_packet, raw);
            success = true;
            break;
        }
        case PacketType::TouchState: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            struct {
                s16 x;
                s16 y;
                bool valid;
            } state;
            std::memcpy(&state, data, sizeof(state));
            HandleTouchState(*request_packet, state.x, state.y, state.valid);
            success = true;
            break;
        }
        case PacketType::MotionState: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            struct {
                s16 x;
                s16 y;
                s16 z;
                s16 roll;
                s16 pitch;
                s16 yaw;
            } state;
            std::memcpy(&state, data, sizeof(state));
            HandleMotionState(*request_packet, state.x, state.y, state.z, state.roll, state.pitch,
                              state.yaw);
            success = true;
            break;
        }
        case PacketType::CircleState: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            struct {
                s16 x;
                s16 y;
            } state;
            std::memcpy(&state, data, sizeof(state));
            HandleCircleState(*request_packet, state.x, state.y);
            success = true;
            break;
        }
        case PacketType::SetResolution: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            u16 resolution;
            std::memcpy(&resolution, data, sizeof(u16));
            HandleSetResolution(*request_packet, resolution);
            success = true;
            break;
        }
        case PacketType::SetApplication: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            std::string path;
            path.resize(request_packet->GetPacketDataSize() - (sizeof(u32) * 2));
            std::memcpy(&path[0], data, request_packet->GetPacketDataSize() - (sizeof(u32) * 2));
            HandleSetApplication(*request_packet, path);
            success = true;
            break;
        }
        case PacketType::SetOverrideControls: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            struct {
                bool pad;
                bool touch;
                bool motion;
                bool circle;
            } state;
            std::memcpy(&state, data, sizeof(state));
            HandleSetOverrideControls(*request_packet, state.pad, state.touch, state.motion,
                                      state.circle);
            success = true;
            break;
        }
        case PacketType::Pause: {
            HandlePause(*request_packet);
            success = true;
            break;
        }
        case PacketType::Resume: {
            HandleResume(*request_packet);
            success = true;
            break;
        }
        case PacketType::Restart: {
            HandleRestart(*request_packet);
            success = true;
            break;
        }
        case PacketType::SetSpeedLimit: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            u16 speed_limit;
            std::memcpy(&speed_limit, data, sizeof(u16));
            HandleSetSpeedLimit(*request_packet, speed_limit);
            success = true;
            break;
        }
        case PacketType::SetBackgroundColor: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            struct {
                float r;
                float g;
                float b;
            } color;
            std::memcpy(&color, data, sizeof(color));
            HandleSetBackgroundColor(*request_packet, color.r, color.g, color.b);
            success = true;
            break;
        }
        case PacketType::SetScreenRefreshRate: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            int rate;
            std::memcpy(&rate, data, sizeof(int));
            HandleSetScreenRefreshRate(*request_packet, rate);
            success = true;
            break;
        }
        case PacketType::IsButtonPressed: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            int button;
            std::memcpy(&button, data, sizeof(int));
            HandleIsButtonPressed(*request_packet, button);
            success = true;
            break;
        }
        case PacketType::SetFrameAdvancing: {
            const u8* data{request_packet->GetPacketData().data() + (sizeof(u32) * 2)};
            bool enabled;
            std::memcpy(&enabled, data, sizeof(bool));
            HandleSetFrameAdvancing(*request_packet, enabled);
            success = true;
            break;
        }
        case PacketType::AdvanceFrame: {
            HandleAdvanceFrame(*request_packet);
            success = true;
            break;
        }
        default:
            break;
        }
    }
    if (!success) {
        // Send an empty reply, so as not to hang the client
        request_packet->SetPacketDataSize(0);
        request_packet->SendReply();
    }
}

void RPCServer::HandleRequestsLoop() {
    std::unique_ptr<RPC::Packet> request_packet;
    LOG_INFO(RPC, "Request handler started.");
    for (;;) {
        request_packet = request_queue.PopWait();
        if (!request_packet)
            break;
        HandleSingleRequest(std::move(request_packet));
    }
}

void RPCServer::QueueRequest(std::unique_ptr<RPC::Packet> request) {
    request_queue.Push(std::move(request));
}

void RPCServer::Start() {
    const auto threadFunction{[this]() { HandleRequestsLoop(); }};
    request_handler_thread = std::thread(threadFunction);
    server.Start();
}

void RPCServer::Stop() {
    server.Stop();
    request_handler_thread.join();
}

} // namespace RPC
