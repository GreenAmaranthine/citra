// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <future>
#include <vector>
#include "announce_multiplayer_session.h"
#include "common/announce_multiplayer_room.h"
#include "common/assert.h"
#include "core/core.h"
#include "lobby/api.h"
#include "network/room.h"

namespace Core {

// Time between room is announced to lobby
constexpr std::chrono::seconds announce_time_interval{15};

AnnounceMultiplayerSession::AnnounceMultiplayerSession(Network::Room& room) : room{room} {
    backend = std::make_unique<LobbyAPI>();
}

void AnnounceMultiplayerSession::Start() {
    if (announce_multiplayer_thread)
        Stop();
    shutdown_event.Reset();
    announce_multiplayer_thread =
        std::make_unique<std::thread>(&AnnounceMultiplayerSession::AnnounceMultiplayerLoop, this);
}

void AnnounceMultiplayerSession::Stop() {
    if (announce_multiplayer_thread) {
        shutdown_event.Set();
        announce_multiplayer_thread->join();
        announce_multiplayer_thread.reset();
        backend->Delete();
    }
}

AnnounceMultiplayerSession::CallbackHandle AnnounceMultiplayerSession::BindErrorCallback(
    std::function<void(const Common::WebResult&)> function) {
    std::lock_guard lock{callback_mutex};
    auto handle{std::make_shared<std::function<void(const Common::WebResult&)>>(function)};
    error_callbacks.insert(handle);
    return handle;
}

void AnnounceMultiplayerSession::UnbindErrorCallback(CallbackHandle handle) {
    std::lock_guard lock{callback_mutex};
    error_callbacks.erase(handle);
}

AnnounceMultiplayerSession::~AnnounceMultiplayerSession() {
    Stop();
}

void AnnounceMultiplayerSession::AnnounceMultiplayerLoop() {
    auto announce{[&] {
        auto room_information{room.GetRoomInformation()};
        auto member_list{room.GetRoomMemberList()};
        backend->SetRoomInformation(room_information.name, room_information.port,
                                    room_information.creator, room_information.description,
                                    room_information.member_slots, Network::NetworkVersion,
                                    room.HasPassword());
        backend->ClearMembers();
        for (const auto& member : member_list)
            backend->AddMember(member.nickname, member.mac_address, member.program);
        auto result{backend->Announce()};
        if (result.result_code != Common::WebResult::Code::Success) {
            std::lock_guard lock{callback_mutex};
            for (auto callback : error_callbacks)
                (*callback)(result);
        }
    }};
    announce();
    auto update_time{std::chrono::steady_clock::now()};
    std::future<Common::WebResult> future;
    while (!shutdown_event.WaitUntil(update_time)) {
        update_time += announce_time_interval;
        if (!room.IsOpen())
            break;
        announce();
    }
}

AnnounceMultiplayerRoom::RoomList AnnounceMultiplayerSession::GetRoomList() {
    return backend->GetRoomList();
}

} // namespace Core
