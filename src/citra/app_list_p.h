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

enum class AppListItemType {
    Application = QStandardItem::UserType + 1,
    CustomDir = QStandardItem::UserType + 2,
    InstalledDir = QStandardItem::UserType + 3,
    SystemDir = QStandardItem::UserType + 4,
    AddDir = QStandardItem::UserType + 5
};

/**
 * Gets the icon from SMDH data.
 * @param smdh SMDH data
 * @param large If true, returns large icon (48x48), otherwise returns small icon (24x24)
 * @return QPixmap icon
 */
static QPixmap GetQPixmapFromSMDH(const Loader::SMDH& smdh, bool large) {
    std::vector<u16> icon_data{smdh.GetIcon(large)};
    const uchar* data{reinterpret_cast<const uchar*>(icon_data.data())};
    int size{large ? 48 : 24};
    QImage icon{data, size, size, QImage::Format::Format_RGB16};
    return QPixmap::fromImage(icon);
}

/**
 * Gets the default icon (for application without valid SMDH)
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
 * Gets the region from SMDH data.
 * @param smdh SMDH data
 * @return QString region
 */
static QString GetRegionFromSMDH(const Loader::SMDH& smdh) {
    const Loader::SMDH::Region region{smdh.GetRegion()};

    switch (region) {
    case Loader::SMDH::Region::Invalid:
        return "Invalid region";
    case Loader::SMDH::Region::Japan:
        return "Japan";
    case Loader::SMDH::Region::NorthAmerica:
        return "North America";
    case Loader::SMDH::Region::Europe:
        return "Europe";
    case Loader::SMDH::Region::Australia:
        return "Australia";
    case Loader::SMDH::Region::China:
        return "China";
    case Loader::SMDH::Region::Korea:
        return "Korea";
    case Loader::SMDH::Region::Taiwan:
        return "Taiwan";
    case Loader::SMDH::Region::RegionFree:
        return "Region free";
    default:
        return "Invalid Region";
    }
}

struct CompatStatus {
    QString text;
    QString tooltip;
};

class AppListItem : public QStandardItem {
public:
    AppListItem() : QStandardItem{} {}
    AppListItem(const QString& string) : QStandardItem{string} {}

    virtual ~AppListItem() override {}
};

/// Application list icon sizes (in px)
static const std::unordered_map<UISettings::AppListIconSize, int> IconSizes{
    {UISettings::AppListIconSize::NoIcon, 0},
    {UISettings::AppListIconSize::SmallIcon, 24},
    {UISettings::AppListIconSize::LargeIcon, 48},
};

/**
 * A specialization of AppListItem for path values.
 * This class ensures that for every full path value it holds, a correct string
 * representation of just the filename (with no extension) will be displayed to the user. If
 * this class receives valid SMDH data, it will also display icons and titles.
 */
class AppListItemPath : public AppListItem {
public:
    AppListItemPath() : AppListItem{} {}

    AppListItemPath(const QString& app_path, const std::vector<u8>& smdh_data, u64 program_id,
                    u64 extdata_id)
        : AppListItem{} {
        setData(app_path, FullPathRole);
        setData(qulonglong(program_id), ProgramIdRole);
        setData(qulonglong(extdata_id), ExtdataIdRole);
        if (UISettings::values.app_list_icon_size == UISettings::AppListIconSize::NoIcon) {
            // Do not display icons
            setData(QPixmap{}, Qt::DecorationRole);
        }
        bool large{UISettings::values.app_list_icon_size == UISettings::AppListIconSize::LargeIcon};
        if (!Loader::IsValidSMDH(smdh_data)) {
            // SMDH isn't valid, set a default icon
            if (UISettings::values.app_list_icon_size != UISettings::AppListIconSize::NoIcon)
                setData(GetDefaultIcon(large), Qt::DecorationRole);
            return;
        }
        Loader::SMDH smdh;
        std::memcpy(&smdh, smdh_data.data(), sizeof(Loader::SMDH));
        // Get icon from SMDH
        if (UISettings::values.app_list_icon_size != UISettings::AppListIconSize::NoIcon)
            setData(GetQPixmapFromSMDH(smdh, large), Qt::DecorationRole);
        // Get short title from SMDH
        setData(GetQStringShortTitleFromSMDH(smdh, Loader::SMDH::TitleLanguage::English),
                TitleRole);
        // Get publisher from SMDH
        setData(GetQStringPublisherFromSMDH(smdh, Loader::SMDH::TitleLanguage::English),
                PublisherRole);
    }

    int type() const override {
        return static_cast<int>(AppListItemType::Application);
    }

    QVariant data(int role) const override {
        if (role == Qt::DisplayRole) {
            std::string path, filename, extension;
            std::string sdmc_dir{FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir,
                                                       Settings::values.sdmc_dir + "/")};
            Common::SplitPath(data(FullPathRole).toString().toStdString(), &path, &filename,
                              &extension);
            const std::unordered_map<UISettings::AppListText, QString> display_texts{
                {UISettings::AppListText::FileName, QString::fromStdString(filename + extension)},
                {UISettings::AppListText::FullPath, data(FullPathRole).toString()},
                {UISettings::AppListText::TitleName, data(TitleRole).toString()},
                {UISettings::AppListText::TitleID,
                 QString::fromStdString(fmt::format("{:016X}", data(ProgramIdRole).toULongLong()))},
                {UISettings::AppListText::Publisher, data(PublisherRole).toString()},
            };
            const QString& row1{display_texts.at(UISettings::values.app_list_row_1)};
            QString row2;
            auto row_2_id{UISettings::values.app_list_row_2};
            if (row_2_id != UISettings::AppListText::NoText)
                row2 = (row1.isEmpty() ? "" : "\n     ") + display_texts.at(row_2_id);
            return row1 + row2;
        } else {
            return AppListItem::data(role);
        }
    }

    static const int FullPathRole{Qt::UserRole + 1};
    static const int TitleRole{Qt::UserRole + 2};
    static const int ProgramIdRole{Qt::UserRole + 3};
    static const int ExtdataIdRole{Qt::UserRole + 4};
    static const int PublisherRole{Qt::UserRole + 5};
};

class AppListItemCompat : public AppListItem {
public:
    AppListItemCompat() : AppListItem{} {}

    explicit AppListItemCompat(u64 program_id) {
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
        return static_cast<int>(AppListItemType::Application);
    }
};

class AppListItemRegion : public AppListItem {
public:
    AppListItemRegion() : AppListItem{} {}

    explicit AppListItemRegion(const std::vector<u8>& smdh_data) {
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
 * A specialization of AppListItem for size values.
 * This class ensures that for every numerical size value it holds (in bytes), a correct
 * human-readable string representation will be displayed to the user.
 */
class AppListItemSize : public AppListItem {
public:
    AppListItemSize() : AppListItem{} {}

    AppListItemSize(const qulonglong size_bytes) : AppListItem{} {
        setData(size_bytes, SizeRole);
    }

    void setData(const QVariant& value, int role) override {
        // By specializing setData for SizeRole, we can ensure that the numerical and string
        // representations of the data are always accurate and in the correct format.
        if (role == SizeRole) {
            qulonglong size_bytes{value.toULongLong()};
            AppListItem::setData(ReadableByteSize(size_bytes), Qt::DisplayRole);
            AppListItem::setData(value, SizeRole);
        } else {
            AppListItem::setData(value, role);
        }
    }

    int type() const override {
        return static_cast<int>(AppListItemType::Application);
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

class AppListDir : public AppListItem {
public:
    AppListDir() : AppListItem{} {}

    explicit AppListDir(UISettings::AppDir& directory,
                        AppListItemType type = AppListItemType::CustomDir)
        : dir_type{type} {
        UISettings::AppDir* app_dir{&directory};
        setData(QVariant::fromValue(app_dir), AppDirRole);
        int icon_size{IconSizes.at(UISettings::values.app_list_icon_size)};
        switch (dir_type) {
        case AppListItemType::InstalledDir:
            setData(QIcon::fromTheme("sd_card").pixmap(icon_size), Qt::DecorationRole);
            setData("Installed Titles", Qt::DisplayRole);
            break;
        case AppListItemType::SystemDir:
            setData(QIcon::fromTheme("chip").pixmap(icon_size), Qt::DecorationRole);
            setData("System Titles", Qt::DisplayRole);
            break;
        case AppListItemType::CustomDir:
            QString icon_name = QFileInfo::exists(app_dir->path) ? "folder" : "bad_folder";
            setData(QIcon::fromTheme(icon_name).pixmap(icon_size), Qt::DecorationRole);
            setData(app_dir->path, Qt::DisplayRole);
            break;
        }
    }

    int type() const override {
        return static_cast<int>(dir_type);
    }

    static const int AppDirRole{Qt::UserRole + 1};

private:
    AppListItemType dir_type;
};

class AppListAddDir : public AppListItem {
public:
    explicit AppListAddDir() : AppListItem{} {
        setData(type(), TypeRole);

        int icon_size{IconSizes.at(UISettings::values.app_list_icon_size)};
        setData(QIcon::fromTheme("plus").pixmap(icon_size), Qt::DecorationRole);
        setData("Add New Application Directory", Qt::DisplayRole);
    }

    int type() const override {
        return static_cast<int>(AppListItemType::AddDir);
    }

private:
    static const int TypeRole{Qt::UserRole + 1};
};

class AppList;
class QHBoxLayout;
class QTreeView;
class QLabel;
class QLineEdit;
class QToolButton;

class AppListSearchField : public QWidget {
    Q_OBJECT

public:
    explicit AppListSearchField(AppList* parent = nullptr);

    void setFilterResult(int visible, int total);

    void clear();
    void setFocus();

    int visible;
    int total;

private:
    class KeyReleaseEater : public QObject {
    public:
        explicit KeyReleaseEater(AppList* applist);

    private:
        AppList* applist;
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
