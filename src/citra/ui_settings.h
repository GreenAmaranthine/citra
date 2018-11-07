// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>
#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include "common/common_types.h"

namespace UISettings {

using ContextualShortcut = std::pair<QString, int>;
using Shortcut = std::pair<QString, ContextualShortcut>;

using Themes = std::array<std::pair<const char*, const char*>, 4>;
extern const Themes themes;

struct AppDir {
    QString path;
    bool deep_scan;
    bool expanded;

    bool operator==(const AppDir& rhs) const {
        return path == rhs.path;
    }

    bool operator!=(const AppDir& rhs) const {
        return !operator==(rhs);
    }
};

enum class AppListIconSize {
    NoIcon,    ///< Don't display icons
    SmallIcon, ///< Display a small (24x24) icon
    LargeIcon, ///< Display a large (48x48) icon
};

enum class AppListText {
    NoText = -1, ///< No text
    FileName,    ///< Display the file name of the entry
    FullPath,    ///< Display the full path of the entry
    TitleName,   ///< Display the name of the title
    TitleID,     ///< Display the title ID
    Publisher,   ///< Display the publisher
};

struct Values {
    QByteArray geometry;
    QByteArray state;
    QByteArray screens_geometry;
    QByteArray applist_header_state;

    bool single_window_mode;
    bool fullscreen;
    bool show_filter_bar;
    bool show_status_bar;

    QString amiibo_dir;
    QString apps_dir;
    QString movies_dir;
    QString ram_dumps_dir;
    QString screenshots_dir;
    QString seeds_dir;

    // Application List
    AppListIconSize app_list_icon_size;
    AppListText app_list_row_1;
    AppListText app_list_row_2;
    bool app_list_hide_no_icon;

    u16 screenshot_resolution_factor;

    QList<UISettings::AppDir> app_dirs;
    QStringList recent_files;

    bool confirm_close;
    bool enable_discord_rpc;

    QString theme;

    // Shortcut name <Shortcut, context>
    std::vector<Shortcut> shortcuts;

    // Multiplayer settings
    QString nickname;
    QString ip;
    QString port;
    QString room_nickname;
    QString room_name;
    quint32 max_members;
    QString room_port;
    uint host_type;
    qulonglong app_id;

    // Logging
    bool show_console;
};

extern Values values;

} // namespace UISettings

Q_DECLARE_METATYPE(UISettings::AppDir*);
