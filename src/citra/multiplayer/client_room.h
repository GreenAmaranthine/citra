// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "citra/multiplayer/chat_room.h"

namespace Ui {
class ClientRoom;
} // namespace Ui

class ClientRoomWindow : public QDialog {
    Q_OBJECT

public:
    explicit ClientRoomWindow(QWidget* parent, Core::System& system);
    ~ClientRoomWindow();

    Core::System& system;

    void SetModPerms(bool is_mod);

public slots:
    void OnRoomUpdate(const Network::RoomInformation&);
    void OnStateChange(const Network::RoomMember::State&);

signals:
    void RoomInformationChanged(const Network::RoomInformation&);
    void StateChanged(const Network::RoomMember::State&);

private:
    void Disconnect();
    void UpdateView();

    QStandardItemModel* member_list;
    std::unique_ptr<Ui::ClientRoom> ui;
};
