// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <map>
#include <unordered_map>
#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QRunnable>
#include <QStandardItem>
#include <QString>
#include <QWidget>
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

static QString GitHubIssue(const QString& repo, int number) {
    return QString(
               "<a href=\"https://github.com/repo/issues/number\"><span style=\"text-decoration: "
               "underline; color:#039be5;\">repo#number</span></a>")
        .replace("repo", repo)
        .replace("number", QString::number(number));
}

static const std::unordered_map<u64, QStringList> compatibility_database{{
    // Homebrew
    {0x000400000F800100, {GitHubIssue("citra-valentin/citra", 451)}},   // FBI
    {0x0004000004395500, {"Citra crashes with Unknown result status"}}, // DSiMenuPlusPlus
    {0x000400000D5CDB00, {"Needs SOC:GetAddrInfo"}},                    // White Haired Cat Girl
    {0x000400000EB00000, {"Load fails (NTR is already running)"}},      // Boot NTR Selector

    // ALL
    {0x00040000001C1E00, {GitHubIssue("citra-emu/citra", 3612)}}, // Detective Pikachu
    {0x0004000000164800, {"Needs Nintendo Network support"}},     // Pokémon Sun
    {0x0004000000175E00, {"Needs Nintendo Network support"}},     // Pokémon Moon
    {0x000400000011C400, {"Needs Nintendo Network support"}},     // Pokémon Omega Ruby
    {0x000400000011C500, {"Needs Nintendo Network support"}},     // Pokémon Alpha Sapphire
    {0x00040000001B5000, {"Needs Nintendo Network support"}},     // Pokémon Ultra Sun
    {0x00040000001B5100, {"Needs Nintendo Network support"}},     // Pokémon Ultra Moon
    {0x0004000000055D00, {GitHubIssue("citra-emu/citra", 3009)}}, // Pokémon X
    {0x0004000000055E00, {GitHubIssue("citra-emu/citra", 3009)}}, // Pokémon Y
    {0x000400000F700E00, {}},                                     // Super Mario World
    {0x000400000F980000, {}},                                     // Test Program (CTRAging)

    // EUR
    {0x00040000001A4900, {}}, // Ever Oasis
    {0x000400000F70CD00, {}}, // Fire Emblem Warriors
    {0x000400000014F200,
     {"Needs Nintendo Network support"}}, // Animal Crossing: Happy Home Designer
    {0x000400000017EB00, {}},             // Hyrule Warriors Legends
    {0x0004000000076500, {}},             // Luigi’s Mansion 2
    {0x00040000001BC600, {}},             // Monster Hunter Stories
    {0x0004000000198F00,
     {"Needs Nintendo Network support"}}, // Animal Crossing: New Leaf - Welcome amiibo
    {0x00040000001A0500, {"Needs Nintendo Network support"}}, // Super Mario Maker for Nintendo 3DS
    {0x0004000000086400, {"Needs Nintendo Network support"}}, // Animal Crossing: New Leaf
    {0x0004000000030C00,
     {"Needs Microphone", "Uses Pedometer"}}, // nintendogs + cats (Golden Retriever & New Friends)
    {0x0004000000033600, {}},                 // The Legend of Zelda: Ocarina of Time 3D
    {0x00040000001A4200, {}},                 // Poochy & Yoshi's Woolly World
    {0x0004000000053F00, {}},                 // Super Mario 3D Land
    {0x000400000008C400,
     {GitHubIssue("citra-valentin/citra", 277), GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}}, // Tomodachi Life
    {0x0004000000177000,
     {"Needs DLP", "Needs Nintendo Network support"}}, // The Legend of Zelda: Tri Force Heroes
    {0x000400000007AF00, {}},                          // New Super Mario Bros. 2
    {0x0004000000137F00, {}},                          // New Super Mario Bros. 2: Special Edition
    {0x00040000001B4F00, {}},                          // Miitopia
    {0x00040000000A9200, {}},                          // PICROSS e
    {0x00040000000CBC00, {}},                          // PICROSS e2
    {0x0004000000102900, {}},                          // PICROSS e3
    {0x0004000000128400, {}},                          // PICROSS e4
    {0x000400000014D200, {}},                          // PICROSS e5
    {0x000400000016E800, {}},                          // PICROSS e6
    {0x00040000001AD600, {}},                          // PICROSS e7
    {0x00040000001CE000, {}},                          // PICROSS e8
    {0x0004000000130600, {GitHubIssue("citra-emu/citra", 3347)}}, // Photos with Mario
    {0x0004000000030700,
     {"Local multiplayer needs DLP", "Needs Nintendo Network support"}}, // Mario Kart 7
    {0x0004000000143D00, {}},                                            // 2048
    {0x0004000000187E00, {}},                                            // Picross 3D: Round 2
    {0x00040000000EE000, {}}, // Super Smash Bros. for Nintendo 3DS
    {0x0004000000125600, {}}, // The Legend of Zelda: Majora's Mask 3D
    {0x000400000010C000, {}}, // Kirby: Triple Deluxe
    {0x00040000001B2900, {}}, // YO-KAI WATCH 2: Psychic Specters
    {0x00040000001CB200, {}}, // Captain Toad: Treasure Tracker
    {0x0004000000116700, {}}, // Cut the Rope: Triple Treat
    {0x0004000000030200, {}}, // Kid Icarus Uprising
    {0x00040000000A5F00, {}}, // Paper Mario Sticker Star
    {0x0004000000031600,
     {"Needs Microphone", "Uses Pedometer"}}, // nintendogs + cats (Toy Poodle & New Friends)
    {0x0004000000170B00, {"Softlocks when selecting a game"}}, // Teenage Mutant Ninja Turtles:
                                                               // Master Splinter's Training Pack

    // EUR (System)
    {0x0004001000022A00, {}},                                     // ???
    {0x0004001000022700, {GitHubIssue("citra-emu/citra", 3897), GitHubIssue("citra-emu/citra", 3903)}}, // Mii Maker
    {0x0004001000022100, {"Needs DLP"}},                          // Download Play
    {0x0004001000022300, {}},                                     // Health and Safety Information

    // EUR (Demos)
    {0x00040002001CB201, {}}, // Captain Toad Demo

    // USA
    {0x00040000000E5D00, {}},                                 // PICROSS e
    {0x00040000000CD400, {}},                                 // PICROSS e2
    {0x0004000000101D00, {}},                                 // PICROSS e3
    {0x0004000000127300, {}},                                 // PICROSS e4
    {0x0004000000149800, {}},                                 // PICROSS e5
    {0x000400000016EF00, {}},                                 // PICROSS e6
    {0x00040000001ADB00, {}},                                 // PICROSS e7
    {0x00040000001CF700, {}},                                 // PICROSS e8
    {0x00040000001A0400, {"Needs Nintendo Network support"}}, // Super Mario Maker for Nintendo 3DS
    {0x000400000014F100,
     {"Needs Nintendo Network support"}}, // Animal Crossing: Happy Home Designer
    {0x000400000017F200, {}},             // Moco Moco Friends
    {0x00040000001B4E00, {}},             // Miitopia
    {0x0004000000130500, {GitHubIssue("citra-emu/citra", 3348)}}, // Photos with Mario
    {0x0004000000086300, {"Needs Nintendo Network support"}},     // Animal Crossing: New Leaf
    {0x000400000008C300,
     {GitHubIssue("citra-valentin/citra", 277), GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}}, // Tomodachi Life
    {0x0004000000030800,
     {"Local multiplayer needs DLP", "Needs Nintendo Network support"}}, // Mario Kart 7
    {0x0004000000139000, {}},                                            // 2048
    {0x00040000001B2700, {}},                                  // YO-KAI WATCH 2: Psychic Specters
    {0x0004000000112600, {}},                                  // Cut the Rope: Triple Treat
    {0x00040000001B8700, {}},                                  // Minecraft
    {0x0004000000062300, {"Multiplayer uses deprecated UDS"}}, // Sonic Generations
    {0x0004000000126300, {}},                                  // MONSTER HUNTER 4 ULTIMATE
    {0x00040000001D1900, {GitHubIssue("citra-emu/citra", 4330)}}, // Luigi's Mansion

    // USA (Demos)
    {0x00040002001CB101, {}}, // Captain Toad Demo

    // JPN
    {0x0004000000178800, {}}, // Miitopia(ミートピア)
    {0x0004000000181000, {}}, // My Melody Negai ga Kanau Fushigi na Hako
    {0x0004000000086200, {}}, // とびだせ どうぶつの森
    {0x0004000000198D00, {"Needs Nintendo Network support"}}, // とびだせ どうぶつの森 amiibo+
    {0x00040000001A0300,
     {"Needs Nintendo Network support"}}, // スーパーマリオメーカー for ニンテンドー3DS
    {0x0004000000030600, {}},             // マリオカート7
    {0x000400000014AF00, {}},             // 2048

    // JPN (Demos)
    {0x00040002001CB001, {}}, // Captain Toad Demo

    // KOR
    {0x0004000000086500, {"Needs Nintendo Network support"}}, // 튀어나와요 동물의 숲
    {0x0004000000199000,
     {"Needs Nintendo Network support"}}, // Animal Crossing: New Leaf - Welcome amiibo
    {0x00040000001BB800, {"Needs Nintendo Network support"}}, // Super Mario Maker for Nintendo 3DS
}};

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
 * Gets the short game title from SMDH data.
 * @param smdh SMDH data
 * @param language title language
 * @return QString short title
 */
static QString GetQStringShortTitleFromSMDH(const Loader::SMDH& smdh,
                                            Loader::SMDH::TitleLanguage language) {
    return QString::fromUtf16(smdh.GetShortTitle(language).data());
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

        if (!UISettings::values.game_list_icon_size) {
            // Do not display icons
            setData(QPixmap(), Qt::DecorationRole);
        }

        bool large{UISettings::values.game_list_icon_size == 2};

        if (!Loader::IsValidSMDH(smdh_data)) {
            // SMDH is not valid, set a default icon
            if (UISettings::values.game_list_icon_size)
                setData(GetDefaultIcon(large), Qt::DecorationRole);
            return;
        }

        Loader::SMDH smdh;
        memcpy(&smdh, smdh_data.data(), sizeof(Loader::SMDH));

        // Get icon from SMDH
        if (UISettings::values.game_list_icon_size)
            setData(GetQPixmapFromSMDH(smdh, large), Qt::DecorationRole);

        // Get title from SMDH
        setData(GetQStringShortTitleFromSMDH(smdh, Loader::SMDH::TitleLanguage::English),
                TitleRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::Game);
    }

    QVariant data(int role) const override {
        if (role == Qt::DisplayRole) {
            std::string path, filename, extension;
            std::string sdmc_dir{
                FileUtil::GetUserPath(D_SDMC_IDX, Settings::values.sdmc_dir + "/")};
            Common::SplitPath(data(FullPathRole).toString().toStdString(), &path, &filename,
                              &extension);

            const std::array<QString, 4> display_texts{{
                QString::fromStdString(filename + extension), // file name
                data(FullPathRole).toString(),                // full path
                data(TitleRole).toString(),                   // title name
                QString::fromStdString(
                    fmt::format("{:016X}", data(ProgramIdRole).toULongLong())), // title id
            }};

            const QString& row1{display_texts.at(UISettings::values.game_list_row_1)};

            QString row2;
            int row_2_id{UISettings::values.game_list_row_2};
            if (row_2_id != -1) {
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
            memcpy(&smdh, smdh_data.data(), sizeof(Loader::SMDH));

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

        constexpr std::array<int, 3> icon_sizes{{0, 24, 48}};

        int icon_size{icon_sizes[UISettings::values.game_list_icon_size]};
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

        constexpr std::array<int, 3> icon_sizes{{0, 24, 48}};
        int icon_size{icon_sizes[UISettings::values.game_list_icon_size]};
        setData(QIcon::fromTheme("plus").pixmap(icon_size), Qt::DecorationRole);
        setData("Add New Game Directory", Qt::DisplayRole);
    }

    int type() const override {
        return static_cast<int>(GameListItemType::AddDir);
    }

private:
    static const int TypeRole{Qt::UserRole + 1};
};

/**
 * Asynchronous worker object for populating the game list.
 * Communicates with other threads through Qt's signal/slot system.
 */
class GameListWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit GameListWorker(QList<UISettings::GameDir>& game_dirs)
        : QObject{}, QRunnable{}, game_dirs{game_dirs} {}

public slots:
    /// Starts the processing of directory tree information.
    void run() override;

    /// Tells the worker that it should no longer continue processing. Thread-safe.
    void Cancel();

signals:
    /**
     * The `EntryReady` signal is emitted once an entry has been prepared and is ready
     * to be added to the game list.
     * @param entry_items a list with `QStandardItem`s that make up the columns of the new
     * entry.
     */
    void DirEntryReady(GameListDir* entry_items);
    void EntryReady(QList<QStandardItem*> entry_items, GameListDir* parent_dir);

    /**
     * After the worker has traversed the game directory looking for entries, this signal is
     * emitted with a list of folders that should be watched for changes as well.
     */
    void Finished(QStringList watch_list);

private:
    QStringList watch_list{};
    QList<UISettings::GameDir>& game_dirs;
    std::atomic_bool stop_processing{false};

    void AddFstEntriesToGameList(const std::string& dir_path, unsigned int recursion,
                                 GameListDir* parent_dir);
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
        GameList* gamelist{};
        QString edit_filter_text_old{};

    protected:
        // EventFilter in order to process systemkeys while editing the searchfield
        bool eventFilter(QObject* obj, QEvent* event) override;
    };

    QHBoxLayout* layout_filter{};
    QTreeView* tree_view{};
    QLabel* label_filter{};
    QLineEdit* edit_filter{};
    QLabel* label_filter_result{};
    QToolButton* button_filter_close{};
};
