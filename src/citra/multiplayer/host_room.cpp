// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <future>
#include <QColor>
#include <QImage>
#include <QInputDialog>
#include <QList>
#include <QLocale>
#include <QMetaType>
#include <QTime>
#include <QtConcurrent/QtConcurrentRun>
#include "citra/app_list_p.h"
#include "citra/main.h"
#include "citra/multiplayer/host_room.h"
#include "citra/multiplayer/message.h"
#include "citra/multiplayer/state.h"
#include "citra/multiplayer/validation.h"
#include "citra/ui_settings.h"
#include "common/logging/log.h"
#include "core/announce_multiplayer_session.h"
#include "core/settings.h"
#include "ui_host_room.h"

HostRoomWindow::HostRoomWindow(QWidget* parent, QStandardItemModel* list,
                               std::shared_ptr<Core::AnnounceMultiplayerSession> session)
    : QDialog{parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint},
      ui{std::make_unique<Ui::HostRoom>()}, announce_multiplayer_session{session} {
    ui->setupUi(this);
    // Set up validation for all of the fields
    ui->room_name->setValidator(validation.GetRoomName());
    ui->username->setValidator(validation.GetNickname());
    ui->port->setValidator(validation.GetPort());
    ui->port->setPlaceholderText(QString::number(Network::DefaultRoomPort));
    // Create a proxy to the application list to display the list of preferred applications
    app_list = new QStandardItemModel;
    for (int i{}; i < list->rowCount(); i++) {
        auto parent{list->item(i, 0)};
        for (int j{}; j < parent->rowCount(); j++)
            app_list->appendRow(parent->child(j)->clone());
    }
    proxy = new ComboBoxProxyModel;
    proxy->setSourceModel(app_list);
    proxy->sort(0, Qt::AscendingOrder);
    ui->app_list->setModel(proxy);
    // Disable editing replies text
    ui->tableReplies->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Connect all the widgets to the appropriate events
    connect(ui->buttonAddReply, &QPushButton::released, this, &HostRoomWindow::AddReply);
    connect(ui->buttonRemoveReply, &QPushButton::released, this, &HostRoomWindow::RemoveReply);
    connect(ui->host, &QPushButton::released, this, &HostRoomWindow::Host);
    // Restore the settings
    ui->username->setText(UISettings::values.room_nickname);
    if (ui->username->text().isEmpty() && !Settings::values.citra_username.empty())
        // Use Citra Web Service user name as nickname by default
        ui->username->setText(QString::fromStdString(Settings::values.citra_username));
    ui->room_name->setText(UISettings::values.room_name);
    ui->port->setText(UISettings::values.room_port);
    ui->max_player->setValue(UISettings::values.max_player);
    int index{static_cast<int>(UISettings::values.host_type)};
    if (index < ui->host_type->count())
        ui->host_type->setCurrentIndex(index);
    index = ui->app_list->findData(UISettings::values.app_id, AppListItemPath::ProgramIdRole);
    if (index != -1)
        ui->app_list->setCurrentIndex(index);
}

HostRoomWindow::~HostRoomWindow() = default;

void HostRoomWindow::Host() {
    if (!ui->username->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::USERNAME_NOT_VALID);
        return;
    }
    if (!ui->room_name->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::ROOMNAME_NOT_VALID);
        return;
    }
    if (!ui->port->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::PORT_NOT_VALID);
        return;
    }
    if (auto member{Network::GetRoomMember().lock()}) {
        if (member->GetState() == Network::RoomMember::State::Joining)
            return;
        else if (member->GetState() == Network::RoomMember::State::Joined) {
            auto parent{static_cast<MultiplayerState*>(parentWidget())};
            if (!parent->OnCloseRoom()) {
                close();
                return;
            }
        }
        ui->host->setDisabled(true);
        auto app_name{ui->app_list->currentData(Qt::DisplayRole).toString()};
        auto app_id{ui->app_list->currentData(AppListItemPath::ProgramIdRole).toLongLong()};
        auto port{ui->port->isModified() ? ui->port->text().toInt() : Network::DefaultRoomPort};
        auto password{ui->password->text().toStdString()};
        if (auto room{Network::GetRoom().lock()}) {
            bool created{room->Create(ui->room_name->text().toStdString(), "", port, password,
                                      ui->max_player->value(), app_name.toStdString(), app_id)};
            if (!created) {
                NetworkMessage::ShowError(NetworkMessage::COULD_NOT_CREATE_ROOM);
                LOG_ERROR(Network, "Could not create room!");
                ui->host->setEnabled(true);
                return;
            }
        }
        member->Join(ui->username->text().toStdString(), "127.0.0.1", port, BroadcastMac, password);
        // Store settings
        UISettings::values.room_nickname = ui->username->text();
        UISettings::values.room_name = ui->room_name->text();
        UISettings::values.app_id =
            ui->app_list->currentData(AppListItemPath::ProgramIdRole).toLongLong();
        UISettings::values.max_player = ui->max_player->value();
        UISettings::values.host_type = ui->host_type->currentIndex();
        UISettings::values.room_port = (ui->port->isModified() && !ui->port->text().isEmpty())
                                           ? ui->port->text()
                                           : QString::number(Network::DefaultRoomPort);
        OnConnection();
    }
}

void HostRoomWindow::AddReply() {
    QString message{QInputDialog::getText(this, "Add Reply", "Message:")};
    if (message.isEmpty())
        return;
    QString reply{QInputDialog::getText(this, "Add Reply", "Reply:")};
    if (reply.isEmpty())
        return;
    int row{ui->tableReplies->rowCount()};
    ui->tableReplies->insertRow(row);
    ui->tableReplies->setItem(row, 0 /* Message */, new QTableWidgetItem(message));
    ui->tableReplies->setItem(row, 1 /* Reply */, new QTableWidgetItem(reply));
    ui->buttonRemoveReply->setEnabled(true);
    UpdateReplies();
}

void HostRoomWindow::RemoveReply() {
    for (const auto& item : ui->tableReplies->selectedItems())
        ui->tableReplies->removeRow(item->row());
    UpdateReplies();
    if (ui->tableReplies->rowCount() == 0)
        ui->buttonRemoveReply->setEnabled(false);
}

void HostRoomWindow::UpdateReplies() {
    // Only update replies if the user is hosting a room.
    if (auto room{Network::GetRoom().lock()})
        if (room->IsOpen()) {
            auto parent{static_cast<MultiplayerState*>(parentWidget())};
            MultiplayerState::Replies replies;
            for (int i{}; i < ui->tableReplies->rowCount(); ++i)
                replies.emplace(ui->tableReplies->item(i, /* Message */ 0)->text().toStdString(),
                                ui->tableReplies->item(i, /* Reply */ 1)->text().toStdString());
            parent->SetReplies(replies);
        }
}

void HostRoomWindow::OnConnection() {
    ui->host->setEnabled(true);
    if (auto member{Network::GetRoomMember().lock()}) {
        if (member->GetState() == Network::RoomMember::State::Joining) {
            // Start the announce session if they chose Public
            if (ui->host_type->currentIndex() == 0)
                if (auto session{announce_multiplayer_session.lock()})
                    session->Start();
                else
                    LOG_ERROR(Network, "Starting announce session failed");
            UpdateReplies();
            close();
        }
    }
}

QVariant ComboBoxProxyModel::data(const QModelIndex& idx, int role) const {
    if (role != Qt::DisplayRole) {
        auto val{QSortFilterProxyModel::data(idx, role)};
        // If its the icon, shrink it to 16x16
        if (role == Qt::DecorationRole)
            val = val.value<QImage>().scaled(16, 16, Qt::KeepAspectRatio);
        return val;
    }
    std::string filename;
    Common::SplitPath(
        QSortFilterProxyModel::data(idx, AppListItemPath::FullPathRole).toString().toStdString(),
        nullptr, &filename, nullptr);
    QString title{QSortFilterProxyModel::data(idx, AppListItemPath::TitleRole).toString()};
    return title.isEmpty() ? QString::fromStdString(filename) : title;
}

bool ComboBoxProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    auto leftData = left.data(AppListItemPath::TitleRole).toString();
    auto rightData = right.data(AppListItemPath::TitleRole).toString();
    return leftData.compare(rightData) < 0;
}
