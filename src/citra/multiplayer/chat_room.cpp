// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <future>
#include <QColor>
#include <QImage>
#include <QLabel>
#include <QList>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QMetaType>
#include <QTime>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include "citra/multiplayer/chat_room.h"
#include "citra/multiplayer/client_room.h"
#include "citra/multiplayer/emojis.h"
#include "citra/multiplayer/message.h"
#include "citra/multiplayer/state.h"
#include "citra/program_list_p.h"
#include "common/logging/log.h"
#include "core/announce_multiplayer_session.h"
#include "core/core.h"
#include "ui_chat_room.h"

class ChatMessage {
public:
    explicit ChatMessage(const Network::ChatEntry& chat, QTime ts = {}) {
        /// Convert the time to their default locale defined format
        QLocale locale;
        timestamp = locale.toString(ts.isValid() ? ts : QTime::currentTime(), QLocale::ShortFormat);
        nickname = QString::fromStdString(chat.nickname);
        message = QString::fromStdString(chat.message);
    }

    /// Format the message using the member's color
    QString GetMemberChatMessage(u16 member) const {
        auto color{member_color[member % 16]};
        return QString("[%1] <font color='%2'>&lt;%3&gt;</font> %4")
            .arg(timestamp, color, nickname.toHtmlEscaped(), message.toHtmlEscaped());
    }

private:
    static constexpr std::array<const char*, 16> member_color{
        {"#0000FF", "#FF0000", "#8A2BE2", "#FF69B4", "#1E90FF", "#008000", "#00FF7F", "#B22222",
         "#DAA520", "#FF4500", "#2E8B57", "#5F9EA0", "#D2691E", "#9ACD32", "#FF7F50", "FFFF00"}};

    QString timestamp;
    QString nickname;
    QString message;
};

class StatusMessage {
public:
    explicit StatusMessage(const QString& msg, QTime ts = {}) {
        /// Convert the time to their default locale defined format
        QLocale locale;
        timestamp = locale.toString(ts.isValid() ? ts : QTime::currentTime(), QLocale::ShortFormat);
        message = msg;
    }

    QString GetSystemChatMessage() const {
        return QString("[%1] <font color='#888888'><i>%2</i></font>").arg(timestamp, message);
    }

private:
    QString timestamp;
    QString message;
};

ChatRoom::ChatRoom(QWidget* parent)
    : QWidget{parent}, ui{std::make_unique<Ui::ChatRoom>()},
      system{static_cast<ClientRoomWindow*>(parentWidget())->system} {
    ui->setupUi(this);
    // Set the item_model for member_view
    enum {
        COLUMN_NAME,
        COLUMN_APP,
        COLUMN_COUNT, // Number of columns
    };
    member_list = new QStandardItemModel(ui->member_view);
    ui->member_view->setModel(member_list);
    ui->member_view->setContextMenuPolicy(Qt::CustomContextMenu);
    member_list->insertColumns(0, COLUMN_COUNT);
    member_list->setHeaderData(COLUMN_NAME, Qt::Horizontal, "Name");
    member_list->setHeaderData(COLUMN_APP, Qt::Horizontal, "Program");
    constexpr int MaxChatLines{1000};
    ui->chat_history->document()->setMaximumBlockCount(MaxChatLines);
    // Register the network structs to use in slots and signals
    qRegisterMetaType<Network::ChatEntry>();
    qRegisterMetaType<Network::RoomInformation>();
    qRegisterMetaType<Network::RoomMember::State>();
    // Setup the callbacks for network updates
    system.RoomMember().BindOnChatMessageRecieved(
        [this](const Network::ChatEntry& chat) { emit ChatReceived(chat); });
    connect(this, &ChatRoom::ChatReceived, this, &ChatRoom::OnChatReceive);
    // Load emoji list
    QStringList emoji_list;
    for (const auto& emoji : EmojiMap)
        emoji_list.append(QString(":%1:").arg(QString::fromStdString(emoji.first)));
    completer = new DelimitedCompleter(ui->chat_message, ' ', std::move(emoji_list));
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    // Connect all the widgets to the appropriate events
    connect(ui->member_view, &QTreeView::customContextMenuRequested, this,
            &ChatRoom::PopupContextMenu);
    connect(ui->chat_message, &QLineEdit::returnPressed, ui->send_message, &QPushButton::released);
    connect(ui->chat_message, &QLineEdit::textChanged, this, &ChatRoom::OnChatTextChanged);
    connect(ui->send_message, &QPushButton::released, this, &ChatRoom::OnSendChat);
}

ChatRoom::~ChatRoom() = default;

void ChatRoom::Clear() {
    ui->chat_history->clear();
    block_list.clear();
}

void ChatRoom::AppendStatusMessage(const QString& msg) {
    ui->chat_history->append(StatusMessage(msg).GetSystemChatMessage());
}

bool ChatRoom::Send(QString msg) {
    // Check if we're in a room
    auto& member{system.RoomMember()};
    if (member.GetState() != Network::RoomMember::State::Joined)
        return false;
    // Replace emojis
    for (const auto& emoji : EmojiMap)
        msg.replace(QString(":%1:").arg(QString::fromStdString(emoji.first)),
                    QString::fromStdString(emoji.second));
    // Validate and send message
    auto message{std::move(msg).toStdString()};
    if (!ValidateMessage(message))
        return false;
    auto nick{member.GetNickname()};
    Network::ChatEntry chat{nick, message};
    auto members{member.GetMemberInformation()};
    auto it{std::find_if(members.begin(), members.end(),
                         [&chat](const Network::RoomMember::MemberInformation& member) {
                             return member.nickname == chat.nickname;
                         })};
    if (it == members.end())
        LOG_INFO(Network, "Cannot find self in the member list when sending a message.");
    auto member_id{std::distance(members.begin(), it)};
    ChatMessage m{chat};
    member.SendChatMessage(message);
    AppendChatMessage(m.GetMemberChatMessage(member_id));
    return true;
}

void ChatRoom::HandleNewMessage(const QString& msg) {
    const auto& replies{static_cast<MultiplayerState*>(parentWidget()->parent())->GetReplies()};
    auto itr{replies.find(msg.toStdString())};
    if (itr != replies.end())
        Send(QString::fromStdString(itr->second));
}

void ChatRoom::AppendChatMessage(const QString& msg) {
    ui->chat_history->append(msg);
}

bool ChatRoom::ValidateMessage(const std::string& msg) {
    return !msg.empty();
}

void ChatRoom::Disable() {
    ui->send_message->setDisabled(true);
    ui->chat_message->setDisabled(true);
}

void ChatRoom::Enable() {
    ui->send_message->setEnabled(true);
    ui->chat_message->setEnabled(true);
}

void ChatRoom::OnChatReceive(const Network::ChatEntry& chat) {
    if (!ValidateMessage(chat.message))
        return;
    // Get the ID of the member
    auto members{system.RoomMember().GetMemberInformation()};
    auto it{std::find_if(members.begin(), members.end(),
                         [&chat](const Network::RoomMember::MemberInformation& member) {
                             return member.nickname == chat.nickname;
                         })};
    if (it == members.end()) {
        LOG_INFO(Network, "Chat message received from unknown member. Ignoring it.");
        return;
    }
    if (block_list.count(chat.nickname)) {
        LOG_INFO(Network, "Chat message received from blocked member {}. Ignoring it.",
                 chat.nickname);
        return;
    }
    auto member{std::distance(members.begin(), it)};
    ChatMessage m{chat};
    AppendChatMessage(m.GetMemberChatMessage(member));
    QString message{QString::fromStdString(chat.message)};
    HandleNewMessage(message.remove(QChar('\0')));
}

void ChatRoom::OnSendChat() {
    QString message{ui->chat_message->text()};
    if (!Send(message))
        return;
    ui->chat_message->clear();
    HandleNewMessage(message);
}

void ChatRoom::SetMemberList(const Network::RoomMember::MemberList& member_list) {
    // TODO: Remember which row is selected
    this->member_list->removeRows(0, this->member_list->rowCount());
    for (const auto& member : member_list) {
        if (member.nickname.empty())
            continue;
        QList<QStandardItem*> l;
        std::vector<std::string> elements{member.nickname, member.program_info.name};
        for (const auto& item : elements) {
            QStandardItem* child{new QStandardItem(QString::fromStdString(item))};
            child->setEditable(false);
            l.append(child);
        }
        this->member_list->invisibleRootItem()->appendRow(l);
    }
    // TODO: Restore row selection
}

void ChatRoom::OnChatTextChanged() {
    if (ui->chat_message->text().length() > Network::MaxMessageSize)
        ui->chat_message->setText(ui->chat_message->text().left(Network::MaxMessageSize));
}

void ChatRoom::PopupContextMenu(const QPoint& menu_location) {
    QModelIndex item{ui->member_view->indexAt(menu_location)};
    if (!item.isValid())
        return;
    std::string nickname{member_list->item(item.row())->text().toStdString()};
    // You can't block yourself
    if (nickname == system.RoomMember().GetNickname())
        return;
    QMenu context_menu;
    QAction* block_action{context_menu.addAction("Block Member")};
    block_action->setCheckable(true);
    block_action->setChecked(block_list.count(nickname) > 0);
    connect(block_action, &QAction::triggered, [this, nickname] {
        if (block_list.count(nickname))
            block_list.erase(nickname);
        else {
            QMessageBox::StandardButton result{QMessageBox::question(
                this, "Block Member",
                QString("When you block a member, you will no longer receive chat messages from "
                        "them.<br><br>Are you sure you would like to block %1?")
                    .arg(QString::fromStdString(nickname)),
                QMessageBox::Yes | QMessageBox::No)};
            if (result == QMessageBox::Yes)
                block_list.emplace(nickname);
        }
    });
    context_menu.exec(ui->member_view->viewport()->mapToGlobal(menu_location));
}
