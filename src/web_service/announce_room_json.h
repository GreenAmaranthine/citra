// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <string>
#include "common/announce_multiplayer_room.h"
#include "web_service/web_backend.h"

namespace WebService {

/**
 * Implementation of AnnounceMultiplayerRoom::Backend that (de)serializes room information into/from
 * JSON, and submits/gets it to/from the Citra web service
 */
class RoomJson : public AnnounceMultiplayerRoom::Backend {
public:
    RoomJson(const std::string& host, const std::string& username, const std::string& token)
        : client(host, username, token), host(host), username(username), token(token) {}
    ~RoomJson() = default;
    void SetRoomInformation(const std::string& uid, const std::string& name, const u16 port,
                            const u32 max_members, const u32 net_version, const bool has_password,
                            const std::string& preferred_program,
                            const u64 preferred_program_id) override;
    void AddMember(const std::string& nickname, const MacAddress& mac_address, const u64 program_id,
                   const std::string& program_name) override;
    Common::WebResult Announce() override;
    void ClearMembers() override;
    AnnounceMultiplayerRoom::RoomList GetRoomList() override;
    void Delete() override;

private:
    AnnounceMultiplayerRoom::Room room;
    Client client;
    std::string host;
    std::string username;
    std::string token;
};

} // namespace WebService
