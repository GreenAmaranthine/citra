// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QInputDialog>
#include <QList>
#include <QtConcurrent/QtConcurrentRun>
#include "citra/app_list_p.h"
#include "citra/main.h"
#include "citra/multiplayer/client_room.h"
#include "citra/multiplayer/lobby.h"
#include "citra/multiplayer/lobby_p.h"
#include "citra/multiplayer/message.h"
#include "citra/multiplayer/state.h"
#include "citra/multiplayer/validation.h"
#include "citra/ui_settings.h"
#include "common/logging/log.h"
#include "core/settings.h"
#include "network/room_member.h"

Lobby::Lobby(QWidget* parent, QStandardItemModel* list,
             std::shared_ptr<Core::AnnounceMultiplayerSession> session, Core::System& system)
    : QDialog{parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint},
      ui{std::make_unique<Ui::Lobby>()}, announce_multiplayer_session{session}, system{system} {
    ui->setupUi(this);
    // Setup the watcher for background connections
    watcher = new QFutureWatcher<void>;
    model = new QStandardItemModel(ui->room_list);
    // Create a proxy to the application list to get the list of applications
    app_list = new QStandardItemModel;
    for (int i{}; i < list->rowCount(); i++) {
        auto parent{list->item(i, 0)};
        for (int j{}; j < parent->rowCount(); j++)
            app_list->appendRow(parent->child(j)->clone());
    }
    proxy = new LobbyFilterProxyModel(this, app_list);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortLocaleAware(true);
    ui->room_list->setModel(proxy);
    ui->room_list->header()->setSectionResizeMode(QHeaderView::Interactive);
    ui->room_list->header()->stretchLastSection();
    ui->room_list->setAlternatingRowColors(true);
    ui->room_list->setSelectionMode(QHeaderView::SingleSelection);
    ui->room_list->setSelectionBehavior(QHeaderView::SelectRows);
    ui->room_list->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    ui->room_list->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    ui->room_list->setSortingEnabled(true);
    ui->room_list->setEditTriggers(QHeaderView::NoEditTriggers);
    ui->room_list->setExpandsOnDoubleClick(false);
    ui->room_list->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->nickname->setValidator(validation.GetNickname());
    ui->nickname->setText(UISettings::values.nickname);
    if (ui->nickname->text().isEmpty() && !Settings::values.citra_username.empty())
        // Use Citra Web Service user name as nickname by default
        ui->nickname->setText(QString::fromStdString(Settings::values.citra_username));
    // UI Buttons
    connect(ui->refresh_list, &QPushButton::released, this, &Lobby::RefreshLobby);
    connect(ui->applications_i_have, &QCheckBox::stateChanged, proxy,
            &LobbyFilterProxyModel::SetFilterOwned);
    connect(ui->hide_full, &QCheckBox::stateChanged, proxy, &LobbyFilterProxyModel::SetFilterFull);
    connect(ui->search, &QLineEdit::textChanged, proxy, &LobbyFilterProxyModel::SetFilterSearch);
    connect(ui->room_list, &QTreeView::doubleClicked, this, &Lobby::OnJoinRoom);
    connect(ui->room_list, &QTreeView::clicked, this, &Lobby::OnExpandRoom);
    // Actions
    connect(&room_list_watcher, &QFutureWatcher<AnnounceMultiplayerRoom::RoomList>::finished, this,
            &Lobby::OnRefreshLobby);
    // Manually start a refresh when the window is opening
    // TODO: if this refresh is slow for people with bad internet, then don't do it as
    // part of the constructor, but offload the refresh until after the window shown. perhaps emit a
    // refreshroomlist signal from places that open the lobby
    RefreshLobby();
}

QString Lobby::PasswordPrompt() {
    bool ok{};
    const auto text{QInputDialog::getText(this, "Password Required to Join",
                                          "Password:", QLineEdit::Password, "", &ok)};
    return ok ? text : QString();
}

void Lobby::OnExpandRoom(const QModelIndex& index) {
    QModelIndex member_index{proxy->index(index.row(), Column::MEMBER)};
    auto member_list{proxy->data(member_index, LobbyItemMemberList::MemberListRole).toList()};
}

void Lobby::OnJoinRoom(const QModelIndex& source) {
    // Prevent the user from trying to join a room while they are already joining.
    auto& member{system.RoomMember()};
    if (member.GetState() == Network::RoomMember::State::Joining)
        return;
    else if (member.GetState() == Network::RoomMember::State::Joined)
        // And ask if they want to leave the room if they are already in one.
        if (!NetworkMessage::WarnDisconnect())
            return;
    QModelIndex index{source};
    // If the user double clicks on a child row (aka the member list) then use the parent instead
    if (source.parent() != QModelIndex())
        index = source.parent();
    if (!ui->nickname->hasAcceptableInput()) {
        NetworkMessage::ShowError(NetworkMessage::USERNAME_NOT_VALID);
        return;
    }

    // Get a password to pass if the room is password protected
    QModelIndex password_index{proxy->index(index.row(), Column::ROOM_NAME)};
    bool has_password{proxy->data(password_index, LobbyItemName::PasswordRole).toBool()};
    const std::string password{has_password ? PasswordPrompt().toStdString() : ""};
    if (has_password && password.empty())
        return;

    QModelIndex connection_index{proxy->index(index.row(), Column::HOST)};
    const std::string nickname{ui->nickname->text().toStdString()};
    const std::string ip{
        proxy->data(connection_index, LobbyItemHost::HostIPRole).toString().toStdString()};
    int port{proxy->data(connection_index, LobbyItemHost::HostPortRole).toInt()};
    // Attempt to connect in a different thread
    QFuture<void> f{QtConcurrent::run([this, nickname, ip, port, password] {
        system.RoomMember().Join(nickname, ip.c_str(), port, BroadcastMac, password);
    })};
    watcher->setFuture(f);
    // TODO: disable widgets and display a connecting while we wait
    // Save settings
    UISettings::values.nickname = ui->nickname->text();
    UISettings::values.ip = proxy->data(connection_index, LobbyItemHost::HostIPRole).toString();
    UISettings::values.port = proxy->data(connection_index, LobbyItemHost::HostPortRole).toString();
}

void Lobby::ResetModel() {
    model->clear();
    model->insertColumns(0, Column::TOTAL);
    model->setHeaderData(Column::EXPAND, Qt::Horizontal, "", Qt::DisplayRole);
    model->setHeaderData(Column::ROOM_NAME, Qt::Horizontal, "Room Name", Qt::DisplayRole);
    model->setHeaderData(Column::APP_NAME, Qt::Horizontal, "Preferred Application",
                         Qt::DisplayRole);
    model->setHeaderData(Column::HOST, Qt::Horizontal, "Host", Qt::DisplayRole);
    model->setHeaderData(Column::MEMBER, Qt::Horizontal, "Members", Qt::DisplayRole);
}

void Lobby::RefreshLobby() {
    if (auto session{announce_multiplayer_session.lock()}) {
        ResetModel();
        ui->refresh_list->setEnabled(false);
        ui->refresh_list->setText("Refreshing");
        room_list_watcher.setFuture(
            QtConcurrent::run([session]() { return session->GetRoomList(); }));
    }
    // TODO: Display an error box about announce couldn't be started
}

void Lobby::OnRefreshLobby() {
    AnnounceMultiplayerRoom::RoomList new_room_list{room_list_watcher.result()};
    for (auto room : new_room_list) {
        // Find the icon if this person has that application.
        QPixmap smdh_icon;
        for (int r{}; r < app_list->rowCount(); ++r) {
            auto index{app_list->index(r, 0)};
            auto app_id{app_list->data(index, AppListItemPath::ProgramIdRole).toULongLong()};
            if (app_id != 0 && room.preferred_app_id == app_id)
                smdh_icon = app_list->data(index, Qt::DecorationRole).value<QPixmap>();
        }
        QList<QVariant> members;
        for (auto member : room.members) {
            QVariant var;
            var.setValue(LobbyMember{QString::fromStdString(member.name), member.app_id,
                                     QString::fromStdString(member.app_name)});
            members.append(var);
        }
        auto first_item{new LobbyItem()};
        auto row{QList<QStandardItem*>({
            first_item,
            new LobbyItemName(room.has_password, QString::fromStdString(room.name)),
            new LobbyItemApp(room.preferred_app_id, QString::fromStdString(room.preferred_app),
                             smdh_icon),
            new LobbyItemHost(QString::fromStdString(room.owner), QString::fromStdString(room.ip),
                              room.port),
            new LobbyItemMemberList(members, room.max_members),
        })};
        model->appendRow(row);
        // To make the rows expandable, add the member data as a child of the first column of the
        // rows with people in them and have qt set them to colspan after the model is finished
        // resetting
        if (room.members.size() > 0)
            first_item->appendRow(new LobbyItemExpandedMemberList(members));
    }
    // Reenable the refresh button and resize the columns
    ui->refresh_list->setEnabled(true);
    ui->refresh_list->setText("Refresh List");
    ui->room_list->header()->stretchLastSection();
    for (int i{}; i < Column::TOTAL - 1; ++i)
        ui->room_list->resizeColumnToContents(i);
    // Set the member list child items to span all columns
    for (int i{}; i < proxy->rowCount(); i++) {
        auto parent{model->item(i, 0)};
        if (parent->hasChildren())
            ui->room_list->setFirstColumnSpanned(0, proxy->index(i, 0), true);
    }
}

LobbyFilterProxyModel::LobbyFilterProxyModel(QWidget* parent, QStandardItemModel* list)
    : QSortFilterProxyModel{parent}, app_list{list} {}

bool LobbyFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    // Prioritize filters by fastest to compute
    // Pass over any child rows (aka row that shows the players in the room)
    if (sourceParent != QModelIndex())
        return true;
    // Filter by filled rooms
    if (filter_full) {
        QModelIndex member_list{sourceModel()->index(sourceRow, Column::MEMBER, sourceParent)};
        int member_count{
            sourceModel()->data(member_list, LobbyItemMemberList::MemberListRole).toList().size()};
        int max_members{
            sourceModel()->data(member_list, LobbyItemMemberList::MaxMemberRole).toInt()};
        if (member_count >= max_members)
            return false;
    }
    // Filter by search parameters
    if (!filter_search.isEmpty()) {
        QModelIndex app_name{sourceModel()->index(sourceRow, Column::APP_NAME, sourceParent)};
        QModelIndex room_name{sourceModel()->index(sourceRow, Column::ROOM_NAME, sourceParent)};
        QModelIndex host_name{sourceModel()->index(sourceRow, Column::HOST, sourceParent)};
        bool preferred_app_match{sourceModel()
                                     ->data(app_name, LobbyItemApp::AppNameRole)
                                     .toString()
                                     .contains(filter_search, filterCaseSensitivity())};
        bool room_name_match{sourceModel()
                                 ->data(room_name, LobbyItemName::NameRole)
                                 .toString()
                                 .contains(filter_search, filterCaseSensitivity())};
        bool username_match{sourceModel()
                                ->data(host_name, LobbyItemHost::HostUsernameRole)
                                .toString()
                                .contains(filter_search, filterCaseSensitivity())};
        if (!preferred_app_match && !room_name_match && !username_match)
            return false;
    }
    // Filter by applications user has
    if (filter_has) {
        QModelIndex app_name{sourceModel()->index(sourceRow, Column::APP_NAME, sourceParent)};
        QList<QModelIndex> apps;
        for (int r{}; r < app_list->rowCount(); ++r)
            apps.append(QModelIndex(app_list->index(r, 0)));
        auto current_id{sourceModel()->data(app_name, LobbyItemApp::TitleIDRole).toLongLong()};
        if (current_id == 0)
            // Homebrew often doesn't have a title ID and this hides them
            return false;
        bool has{};
        for (const auto& app : apps) {
            auto app_id{app_list->data(app, AppListItemPath::ProgramIdRole).toLongLong()};
            if (current_id == app_id)
                has = true;
        }
        if (!has)
            return false;
    }
    return true;
}

void LobbyFilterProxyModel::sort(int column, Qt::SortOrder order) {
    sourceModel()->sort(column, order);
}

void LobbyFilterProxyModel::SetFilterOwned(bool filter) {
    filter_has = filter;
    invalidate();
}

void LobbyFilterProxyModel::SetFilterFull(bool filter) {
    filter_full = filter;
    invalidate();
}

void LobbyFilterProxyModel::SetFilterSearch(const QString& filter) {
    filter_search = filter;
    invalidate();
}
