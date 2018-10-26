// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <future>
#include <QColor>
#include <QImage>
#include <QList>
#include <QLocale>
#include <QMetaType>
#include <QTime>
#include <QtConcurrent/QtConcurrentRun>
#include "citra/game_list_p.h"
#include "citra/multiplayer/client_room.h"
#include "citra/multiplayer/message.h"
#include "citra/multiplayer/state.h"
#include "common/logging/log.h"
#include "core/announce_multiplayer_session.h"
#include "ui_client_room.h"

ClientRoomWindow::ClientRoomWindow(QWidget* parent)
    : QDialog{parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint},
      ui{std::make_unique<Ui::ClientRoom>()} {
    ui->setupUi(this);
    // Setup the callbacks for network updates
    if (auto member{Network::GetRoomMember().lock()}) {
        member->BindOnRoomInformationChanged(
            [this](const Network::RoomInformation& info) { emit RoomInformationChanged(info); });
        member->BindOnStateChanged(
            [this](const Network::RoomMember::State& state) { emit StateChanged(state); });

        connect(this, &ClientRoomWindow::RoomInformationChanged, this,
                &ClientRoomWindow::OnRoomUpdate);
        connect(this, &ClientRoomWindow::StateChanged, this, &::ClientRoomWindow::OnStateChange);
    }
    // TODO: Network was not initialized?
    connect(ui->disconnect, &QPushButton::pressed, [this] { Disconnect(); });
    ui->disconnect->setDefault(false);
    ui->disconnect->setAutoDefault(false);
    UpdateView();
}

ClientRoomWindow::~ClientRoomWindow() = default;

void ClientRoomWindow::OnRoomUpdate(const Network::RoomInformation& info) {
    UpdateView();
}

void ClientRoomWindow::OnStateChange(const Network::RoomMember::State& state) {
    if (state == Network::RoomMember::State::Joined) {
        ui->chat->Clear();
        ui->chat->AppendStatusMessage("Connected");
    }
    UpdateView();
}

void ClientRoomWindow::Disconnect() {
    auto parent{static_cast<MultiplayerState*>(parentWidget())};
    if (parent->OnCloseRoom()) {
        ui->chat->AppendStatusMessage("Disconnected");
        close();
    }
}

void ClientRoomWindow::UpdateView() {
    if (auto member{Network::GetRoomMember().lock()})
        if (member->IsConnected()) {
            ui->chat->Enable();
            ui->disconnect->setEnabled(true);
            auto memberlist{member->GetMemberInformation()};
            ui->chat->SetPlayerList(memberlist);
            const auto information{member->GetRoomInformation()};
            setWindowTitle(QString("%1 (%2/%3 members) - connected")
                               .arg(QString::fromStdString(information.name))
                               .arg(memberlist.size())
                               .arg(information.member_slots));
            return;
        }
    // TODO: Can't get RoomMember*, show error and close window
    close();
}
