// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QComboBox>
#include <QFuture>
#include <QIntValidator>
#include <QRegExpValidator>
#include <QString>
#include <QtConcurrent/QtConcurrentRun>
#include "citra_qt/main.h"
#include "citra_qt/multiplayer/client_room.h"
#include "citra_qt/multiplayer/ip_connect.h"
#include "citra_qt/multiplayer/message.h"
#include "citra_qt/multiplayer/state.h"
#include "citra_qt/multiplayer/validation.h"
#include "citra_qt/ui_settings.h"
#include "core/settings.h"
#include "network/network.h"
#include "ui_ip_connect.h"

IpConnectWindow::IpConnectWindow(QWidget* parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint),
      ui(std::make_unique<Ui::IpConnect>()) {

    ui->setupUi(this);

    // setup the watcher for background connections
    watcher = new QFutureWatcher<void>;
    connect(watcher, &QFutureWatcher<void>::finished, this, &IpConnectWindow::OnConnection);

    ui->nickname->setValidator(validation.GetNickname());
    ui->nickname->setText(UISettings::values.nickname);
    ui->ip->setValidator(validation.GetIP());
    ui->ip->setText(UISettings::values.ip);
    ui->port->setValidator(validation.GetPort());
    ui->port->setText(UISettings::values.port);

    connect(ui->connect, &QPushButton::pressed, this, &IpConnectWindow::Connect);
}

IpConnectWindow::~IpConnectWindow() = default;

void IpConnectWindow::Connect() {
    if (!ui->nickname->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::USERNAME_NOT_VALID);
        return;
    }
    if (const auto member = Network::GetRoomMember().lock()) {
        // Prevent the user from trying to join a room while they are already joining.
        if (member->GetState() == Network::RoomMember::State::Joining) {
            return;
        } else if (member->GetState() == Network::RoomMember::State::Joined) {
            // And ask if they want to leave the room if they are already in one.
            if (!NetworkMessage::WarnDisconnect()) {
                return;
            }
        }
    }
    if (!ui->ip->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::IP_ADDRESS_NOT_VALID);
        return;
    }
    if (!ui->port->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::PORT_NOT_VALID);
        return;
    }

    // Store settings
    UISettings::values.nickname = ui->nickname->text();
    UISettings::values.ip = ui->ip->text();
    UISettings::values.port = (ui->port->isModified() && !ui->port->text().isEmpty())
                                  ? ui->port->text()
                                  : UISettings::values.port;
    Settings::Apply();

    // attempt to connect in a different thread
    QFuture<void> f = QtConcurrent::run([&] {
        if (auto room_member = Network::GetRoomMember().lock()) {
            auto port = UISettings::values.port.toUInt();
            room_member->Join(ui->nickname->text().toStdString(),
                              ui->ip->text().toStdString().c_str(), port, 0,
                              Network::NoPreferredMac, ui->password->text().toStdString().c_str());
        }
    });
    watcher->setFuture(f);
    // and disable widgets and display a connecting while we wait
    BeginConnecting();
}

void IpConnectWindow::BeginConnecting() {
    ui->connect->setEnabled(false);
    ui->connect->setText(tr("Connecting"));
}

void IpConnectWindow::EndConnecting() {
    ui->connect->setEnabled(true);
    ui->connect->setText(tr("Connect"));
}

void IpConnectWindow::OnConnection() {
    EndConnecting();
    close();
}
