// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <string>
#include "common/announce_multiplayer_room.h"

namespace httplib {
class SSLClient;
} // namespace httplib

class LobbyAPI {
public:
    LobbyAPI();
    ~LobbyAPI() = default;

    void SetRoomInformation(const std::string& name, const u16 port, const std::string& creator,
                            const std::string& description, const u32 max_members,
                            const u32 net_version, const bool has_password);

    void AddMember(const std::string& nickname, const MACAddress& mac_address,
                   const std::string& program);

    Common::WebResult Announce();
    void ClearMembers();
    AnnounceMultiplayerRoom::RoomList GetRoomList();
    void Delete();

private:
    Common::WebResult MakeRequest(const std::string& method, const std::string& body = {});

    AnnounceMultiplayerRoom::Room room;
    std::shared_ptr<httplib::SSLClient> client;
};
