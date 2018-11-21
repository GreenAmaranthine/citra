// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QVariant>
#include "citra/multiplayer/chat_room.h"
#include "citra/multiplayer/validation.h"

namespace Ui {
class HostRoom;
} // namespace Ui

namespace Core {
class System;
class AnnounceMultiplayerSession;
} // namespace Core

class ConnectionError;
class ComboBoxProxyModel;

class ChatMessage;

class HostRoomWindow : public QDialog {
    Q_OBJECT

public:
    explicit HostRoomWindow(QWidget* parent, QStandardItemModel* list,
                            std::shared_ptr<Core::AnnounceMultiplayerSession> session,
                            Core::System& system);
    ~HostRoomWindow();

private slots:
    /**
     * Handler for connection status changes. Launches the chat window if successful or
     * displays an error
     */
    void OnConnection();

private:
    void Host();
    void AddReply();
    void RemoveReply();
    void UpdateReplies();

    std::weak_ptr<Core::AnnounceMultiplayerSession> announce_multiplayer_session;
    QStandardItemModel* program_list;
    ComboBoxProxyModel* proxy;
    std::unique_ptr<Ui::HostRoom> ui;
    Validation validation;
    Core::System& system;
};

/**
 * Proxy Model for the program list combo box so we can reuse the program list model while
 * still displaying the fields slightly differently
 */
class ComboBoxProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    int columnCount(const QModelIndex& idx) const override {
        return 1;
    }

    QVariant data(const QModelIndex& idx, int role) const override;

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};
