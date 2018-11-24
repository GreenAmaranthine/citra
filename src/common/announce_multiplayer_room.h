// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>
#include "common/common_types.h"
#include "common/web_result.h"

namespace AnnounceMultiplayerRoom {

struct Room {
    struct Member {
        std::string name;
        MACAddress mac_address;
        std::string program;
    };
    std::string name;
    std::string creator;
    std::string description;
    std::string ip;
    u16 port;
    u32 max_members;
    u32 net_version;
    bool has_password;
    bool show;
    std::vector<Member> members;
};
using RoomList = std::vector<Room>;

} // namespace AnnounceMultiplayerRoom
