// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QRunnable>
#include <QStandardItem>
#include <QString>
#include <QWidget>
#include "citra/compatibility.h"
#include "citra/ui_settings.h"
#include "citra/util/util.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/loader/smdh.h"
#include "core/settings.h"

enum class GameListItemType {
    Game = QStandardItem::UserType + 1,
    CustomDir = QStandardItem::UserType + 2,
    InstalledDir = QStandardItem::UserType + 3,
    SystemDir = QStandardItem::UserType + 4,
    AddDir = QStandardItem::UserType + 5
};

/**
 * Gets the game icon from SMDH data.
 * @param smdh SMDH data
 * @param large If true, returns large icon (48x48), otherwise returns small icon (24x24)
 * @return QPixmap game icon
 */
static QPixmap GetQPixmapFromSMDH(const Loader::SMDH& smdh, bool large) {
    std::vector<u16> icon_data{smdh.GetIcon(large)};
    const uchar* data{reinterpret_cast<const uchar*>(icon_data.data())};
    int size{large ? 48 : 24};
    QImage icon{data, size, size, QImage::Format::Format_RGB16};
    return QPixmap::fromImage(icon);
}

/**
 * Gets the default icon (for games without valid SMDH)
 * @param large If true, returns large icon (48x48), otherwise returns small icon (24x24)
 * @return QPixmap default icon
 */
static QPixmap GetDefaultIcon(bool large) {
    int size{large ? 48 : 24};
    QPixmap icon{size, size};
    icon.fill(Qt::transparent);
    return icon;
}

/**
 * Gets the short title from SMDH data.
 * @param smdh SMDH data
 * @param language title language
 * @return QString short title
 */
static QString GetQStringShortTitleFromSMDH(const Loader::SMDH& smdh,
                                            Loader::SMDH::TitleLanguage language) {
    return QString::fromUtf16(smdh.GetShortTitle(language).data());
}

/**
 * Gets the long title from SMDH data.
 * @param smdh SMDH data
 * @param language title language
 * @return QString sholongrt title
 */
static QString GetQStringLongTitleFromSMDH(const Loader::SMDH& smdh,
                                           Loader::SMDH::TitleLanguage language) {
    return QString::fromUtf16(smdh.GetLongTitle(language).data());
}

/**
 * Gets the publisher from SMDH data.
 * @param smdh SMDH data
 * @param language title language
 * @return QString publisher
 */
static QString GetQStringPublisherFromSMDH(const Loader::SMDH& smdh,
                                           Loader::SMDH::TitleLanguage language) {
    return QString::fromUtf16(smdh.GetPublisher(language).data());
}

/**
 * Gets the game region from SMDH data.
 * @param smdh SMDH data
 * @return QString region
 */
static QString GetRegionFromSMDH(const Loader::SMDH& smdh) {
    const Loader::SMDH::GameRegion region{smdh.GetRegion()};

    switch (region) {
    case Loader::SMDH::GameRegion::Invalid:
        return "Invalid region";
    case Loader::SMDH::GameRegion::Japan:
        return "Japan";
    case Loader::SMDH::GameRegion::NorthAmerica:
        return "North America";
    case Loader::SMDH::GameRegion::Europe:
        return "Europe";
    case Loader::SMDH::GameRegion::Australia:
        return "Australia";
    case Loader::SMDH::GameRegion::China:
        return "China";
    case Loader::SMDH::GameRegion::Korea:
        return "Korea";
    case Loader::SMDH::GameRegion::Taiwan:
        return "Taiwan";
    case Loader::SMDH::GameRegion::RegionFree:
        return "Region free";
    default:
        return "Invalid Region";
    }
}

struct CompatStatus {
    QString text;
    QString tooltip;
};

class GameListItem : public QStandardItem {
public:
    GameListItem() : QStandardItem{} {}
    GameListItem(const QString& string) : QStandardItem{string} {}

    virtual ~GameListItem() override {}
};

/// Game list icon sizes (in px)
static const std::unordered_map<UISettings::GameListIconSize, int> IconSizes{
    {UISettings::GameListIconSize::NoIcon, 0},
    {UISettings::GameListIconSize::SmallIcon, 24},
    {UISettings::GameListIconSize::LargeIcon, 48},
};

/**
 * A specialization of GameListItem for path values.
 * This class ensures that for every full path value it holds, a correct string
 * representation of just the filename (with no extension) will be displayed to the user. If
 * this class receives valid SMDH data, it will also display game icons and titles.
 */
class GameListItemPath : public GameListItem {
public:
    GameListItemPath() : GameListItem{} {}

    GameListItemPath(const QString& game_path, const std::vector<u8>& smdh_data, u64 program_id,
                     u64 extdata_id)
        : GameListItem{} {
        setData(game_path, FullPathRole);
        setData(qulonglong(program_id), ProgramIdRole);
        setData(qulonglong(extdata_id), ExtdataIdRole);

        if (UISettings::values.game_list_icon_size == UISettings::GameListIconSize::NoIcon) {
            // Do not display icons
            setData(QPixmap{}, Qt::DecorationRole);
        }

        bool large{UISettings::values.game_list_icon_size ==
                   UISettings::GameListIconSize::LargeIcon};

        if (!Loader::IsValidSMDH(smdh_data)) {
            // SMDH is not valid, set a default icon
            if (UISettings::values.game_list_icon_size != UISettings::GameListIconSize::NoIcon)
                setData(GetDefaultIcon(large), Qt::DecorationRole);
            return;
        }

        Loader::SMDH smdh;
        std::memcpy(&smdh, smdh_data.data(), sizeof(Loader::SMDH));

        // Get icon from SMDH
        if (UISettings::values.game_list_icon_size != UISettings::GameListIconSize::NoIcon)
            setData(GetQPixmapFromSMDH(smdh, large), Qt::DecorationRole);

        // Get short title from SMDH
        setData(GetQStringShortTitleFromSMDH(smdh, Loader::SMDH::TitleLanguage::English),
                TitleRole);

        // Get publisher from SMDH
        setData(GetQStringPublisherFromSMDH(smdh, Loader::SMDH::TitleLanguage::English),
                PublisherRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }

    QVariant data(int role) const override {
        if (role == Qt::DisplayRole) {
            std::string path, filename, extension;
            std::string sdmc_dir{FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir,
                                                       Settings::values.sdmc_dir + "/")};
            Common::SplitPath(data(FullPathRole).toString().toStdString(), &path, &filename,
                              &extension);

            const std::unordered_map<UISettings::GameListText, QString> display_texts{
                {UISettings::GameListText::FileName, QString::fromStdString(filename + extension)},
                {UISettings::GameListText::FullPath, data(FullPathRole).toString()},
                {UISettings::GameListText::TitleName, data(TitleRole).toString()},
                {UISettings::GameListText::TitleID,
                 QString::fromStdString(fmt::format("{:016X}", data(ProgramIdRole).toULongLong()))},
                {UISettings::GameListText::Publisher, data(PublisherRole).toString()},
            };

            const QString& row1{display_texts.at(UISettings::values.game_list_row_1)};

            QString row2;
            auto row_2_id{UISettings::values.game_list_row_2};
            if (row_2_id != UISettings::GameListText::NoText) {
                row2 = (row1.isEmpty() ? "" : "\n     ") + display_texts.at(row_2_id);
            }
            return row1 + row2;
        } else {
            return GameListItem::data(role);
        }
    }

    static const int FullPathRole{Qt::UserRole + 1};
    static const int TitleRole{Qt::UserRole + 2};
    static const int ProgramIdRole{Qt::UserRole + 3};
    static const int ExtdataIdRole{Qt::UserRole + 4};
    static const int PublisherRole{Qt::UserRole + 5};
};

class GameListItemCompat : public GameListItem {
public:
    GameListItemCompat() : GameListItem{} {}

    explicit GameListItemCompat(u64 program_id) {
        auto it{compatibility_database.find(program_id)};
        if (it == compatibility_database.end() || it->second.empty()) {
            setText("0 Issues");
        } else {
            setText(QString("%1 Issue%2")
                        .arg(QString::number(it->second.size()),
                             it->second.size() == 1 ? QString() : "s"));
        }
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }
};

class GameListItemRegion : public GameListItem {
public:
    GameListItemRegion() : GameListItem{} {}

    explicit GameListItemRegion(const std::vector<u8>& smdh_data) {
        if (!Loader::IsValidSMDH(smdh_data)) {
            setText("Invalid region");
        } else {
            Loader::SMDH smdh;
            std::memcpy(&smdh, smdh_data.data(), sizeof(Loader::SMDH));

            setText(GetRegionFromSMDH(smdh));
        }
    }
};

/**
 * A specialization of GameListItem for size values.
 * This class ensures that for every numerical size value it holds (in bytes), a correct
 * human-readable string representation will be displayed to the user.
 */
class GameListItemSize : public GameListItem {
public:
    GameListItemSize() : GameListItem{} {}

    GameListItemSize(const qulonglong size_bytes) : GameListItem{} {
        setData(size_bytes, SizeRole);
    }

    void setData(const QVariant& value, int role) override {
        // By specializing setData for SizeRole, we can ensure that the numerical and string
        // representations of the data are always accurate and in the correct format.
        if (role == SizeRole) {
            qulonglong size_bytes{value.toULongLong()};
            GameListItem::setData(ReadableByteSize(size_bytes), Qt::DisplayRole);
            GameListItem::setData(value, SizeRole);
        } else {
            GameListItem::setData(value, role);
        }
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }

    /**
     * This operator is, in practice, only used by the TreeView sorting systems.
     * Override it so that it will correctly sort by numerical value instead of by string
     * representation.
     */
    bool operator<(const QStandardItem& other) const override {
        return data(SizeRole).toULongLong() < other.data(SizeRole).toULongLong();
    }

private:
    static const int SizeRole{Qt::UserRole + 1};
};

class GameListDir : public GameListItem {
public:
    GameListDir() : GameListItem{} {}

    explicit GameListDir(UISettings::GameDir& directory,
                         GameListItemType type = GameListItemType::CustomDir)
        : dir_type{type} {
        UISettings::GameDir* game_dir{&directory};
        setData(QVariant::fromValue(game_dir), GameDirRole);

        int icon_size{IconSizes.at(UISettings::values.game_list_icon_size)};
        switch (dir_type) {
        case GameListItemType::InstalledDir:
            setData(QIcon::fromTheme("sd_card").pixmap(icon_size), Qt::DecorationRole);
            setData("Installed Titles", Qt::DisplayRole);
            break;
        case GameListItemType::SystemDir:
            setData(QIcon::fromTheme("chip").pixmap(icon_size), Qt::DecorationRole);
            setData("System Titles", Qt::DisplayRole);
            break;
        case GameListItemType::CustomDir:
            QString icon_name = QFileInfo::exists(game_dir->path) ? "folder" : "bad_folder";
            setData(QIcon::fromTheme(icon_name).pixmap(icon_size), Qt::DecorationRole);
            setData(game_dir->path, Qt::DisplayRole);
            break;
        };
    }

    int type() const override {
        return static_cast<int>(dir_type);
    }

    static const int GameDirRole{Qt::UserRole + 1};

private:
    GameListItemType dir_type;
};

class GameListAddDir : public GameListItem {
public:
    explicit GameListAddDir() : GameListItem{} {
        setData(type(), TypeRole);

        int icon_size{IconSizes.at(UISettings::values.game_list_icon_size)};
        setData(QIcon::fromTheme("plus").pixmap(icon_size), Qt::DecorationRole);
        setData("Add New Game Directory", Qt::DisplayRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::AddDir);
    }

private:
    static const int TypeRole{Qt::UserRole + 1};
};

class GameList;
class QHBoxLayout;
class QTreeView;
class QLabel;
class QLineEdit;
class QToolButton;

class GameListSearchField : public QWidget {
    Q_OBJECT

public:
    explicit GameListSearchField(GameList* parent = nullptr);

    void setFilterResult(int visible, int total);

    void clear();
    void setFocus();

    int visible;
    int total;

private:
    class KeyReleaseEater : public QObject {
    public:
        explicit KeyReleaseEater(GameList* gamelist);

    private:
        GameList* gamelist;
        QString edit_filter_text_old;

    protected:
        // EventFilter in order to process systemkeys while editing the searchfield
        bool eventFilter(QObject* obj, QEvent* event) override;
    };

    QHBoxLayout* layout_filter;
    QTreeView* tree_view;
    QLabel* label_filter;
    QLineEdit* edit_filter;
    QLabel* label_filter_result;
    QToolButton* button_filter_close;
};
