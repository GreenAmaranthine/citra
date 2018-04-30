// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include "common/common_types.h"

namespace Network {

constexpr u32 network_version{0xFF01}; ///< The network version
constexpr u16 DefaultRoomPort{24872};
constexpr u32 MaxMessageSize{500};
constexpr u32 MaxConcurrentConnections{
    254};                             ///< Maximum number of concurrent connections allowed rooms.
constexpr std::size_t NumChannels{1}; // Number of channels used for the connection

struct RoomInformation {
    std::string name;        ///< Name of the room
    std::string description; ///< Room description
    u32 member_slots;        ///< Maximum number of members in this room
    u16 port;                ///< The port of this room
    std::string creator;     ///< The creator of this room
};

// The different types of messages that can be sent. The first byte of each packet defines the type
enum RoomMessageTypes : u8 {
    IdJoinRequest = 1,
    IdJoinSuccess,
    IdRoomInformation,
    IdSetProgram,
    IdWifiPacket,
    IdChatMessage,
    IdStatusMessage,
    IdNameCollision,
    IdMacCollision,
    IdVersionMismatch,
    IdWrongPassword,
    IdCloseRoom,
    IdRoomIsFull,
};

/// Types of system status messages
enum StatusMessageTypes : u8 {
    IdMemberJoin = 1, ///< Member joining
    IdMemberLeave,    ///< Member leaving
};

/// This is what a server [person creating a server] would use.
class Room final {
public:
    struct Member {
        std::string nickname;   ///< The nickname of the member.
        std::string program;    ///< The current program of the member.
        MACAddress mac_address; ///< The assigned MAC address of the member.
    };

    Room();
    ~Room();

    /// Return whether the room is open.
    bool IsOpen() const;

    /// Gets the room information of the room.
    const RoomInformation& GetRoomInformation() const;

    /// Gets a list of the mbmers connected to the room.
    std::vector<Member> GetRoomMemberList() const;

    /// Checks if the room is password protected
    bool HasPassword() const;

    /// Creates the socket for this room
    bool Create(const std::string& name, const std::string& description, const std::string& creator,
                u16 port = DefaultRoomPort, const std::string& password = "",
                const u32 max_connections = MaxConcurrentConnections);

    /**
     * Destroys the socket
     */
    void Destroy();

private:
    struct RoomImpl;
    std::unique_ptr<RoomImpl> room_impl;
};

} // namespace Network
