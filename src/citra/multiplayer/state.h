// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <QWidget>
#include "core/announce_multiplayer_session.h"
#include "network/network.h"

class QStandardItemModel;
class Lobby;
class HostRoomWindow;
class ClientRoomWindow;
class DirectConnectWindow;
class ClickableLabel;

class MultiplayerState : public QWidget {
    Q_OBJECT

public:
    using Replies = std::unordered_map<std::string, std::string>;

    explicit MultiplayerState(QWidget* parent, QStandardItemModel* game_list, QAction* leave_room,
                              QAction* show_room);
    ~MultiplayerState();

    /**
     * Close all open multiplayer related dialogs
     */
    void Close();

    ClickableLabel* GetStatusIcon() const {
        return status_icon;
    }

    void SetReplies(Replies replies) {
        this->replies = std::move(replies);
    }

    const Replies& GetReplies() {
        return replies;
    }

public slots:
    void OnNetworkStateChanged(const Network::RoomMember::State& state);
    void OnViewLobby();
    void OnCreateRoom();
    bool OnCloseRoom();
    void OnOpenNetworkRoom();
    void OnDirectConnectToRoom();
    void OnAnnounceFailed(const Common::WebResult&);
    void UpdateThemedIcons();

signals:
    void NetworkStateChanged(const Network::RoomMember::State&);
    void AnnounceFailed(const Common::WebResult&);

private:
    Lobby* lobby{};
    HostRoomWindow* host_room{};
    ClientRoomWindow* client_room{};
    DirectConnectWindow* direct_connect{};
    ClickableLabel* status_icon;
    QStandardItemModel* game_list_model;
    QAction* leave_room;
    QAction* show_room;
    std::shared_ptr<Core::AnnounceMultiplayerSession> announce_multiplayer_session;
    Network::RoomMember::State current_state{Network::RoomMember::State::Uninitialized};
    Network::RoomMember::CallbackHandle<Network::RoomMember::State> state_callback_handle;
    Replies replies;
};

Q_DECLARE_METATYPE(Common::WebResult);
