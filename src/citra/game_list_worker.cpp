// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <QFileInfo>
#include "citra/game_list.h"
#include "citra/game_list_p.h"
#include "citra/game_list_worker.h"
#include "citra/ui_settings.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/fs/archive.h"
#include "core/loader/loader.h"

namespace {

static bool HasSupportedFileExtension(const std::string& file_name) {
    QFileInfo file{QFileInfo(QString::fromStdString(file_name))};
    return GameList::supported_file_extensions.contains(file.suffix(), Qt::CaseInsensitive);
}

} // Anonymous namespace

void GameListWorker::AddFstEntriesToGameList(const std::string& dir_path, unsigned int recursion,
                                             GameListDir* parent_dir) {
    const auto callback{[this, recursion, parent_dir](u64* num_entries_out,
                                                      const std::string& directory,
                                                      const std::string& virtual_name) -> bool {
        std::string physical_name{fmt::format("{}/{}", directory, virtual_name)};

        if (stop_processing) {
            return false; // Breaks the callback loop.
        }

        bool is_dir{FileUtil::IsDirectory(physical_name)};
        if (!is_dir && HasSupportedFileExtension(physical_name)) {
            std::unique_ptr<Loader::AppLoader> loader{Loader::GetLoader(physical_name)};
            if (!loader)
                return true;

            u64 program_id;
            loader->ReadProgramId(program_id);

            u64 extdata_id;
            loader->ReadExtdataId(extdata_id);

            std::vector<u8> smdh{[program_id, &loader]() -> std::vector<u8> {
                std::vector<u8> original_smdh;
                loader->ReadIcon(original_smdh);

                if (program_id < 0x0004000000000000 || program_id > 0x00040000FFFFFFFF)
                    return original_smdh;

                std::string update_path{Service::AM::GetTitleContentPath(
                    Service::FS::MediaType::SDMC, program_id + 0x0000000E00000000)};

                if (!FileUtil::Exists(update_path))
                    return original_smdh;

                std::unique_ptr<Loader::AppLoader> update_loader{Loader::GetLoader(update_path)};

                if (!update_loader)
                    return original_smdh;

                std::vector<u8> update_smdh;
                update_loader->ReadIcon(update_smdh);
                return update_smdh;
            }()};

            if (!Loader::IsValidSMDH(smdh) && UISettings::values.game_list_hide_no_icon) {
                // Skip this invalid entry
                return true;
            }

            emit EntryReady(
                {
                    new GameListItemPath(QString::fromStdString(physical_name), smdh, program_id,
                                         extdata_id),
                    new GameListItemCompat(program_id),
                    new GameListItemRegion(smdh),
                    new GameListItem(
                        QString::fromStdString(Loader::GetFileTypeString(loader->GetFileType()))),
                    new GameListItemSize(FileUtil::GetSize(physical_name)),
                },
                parent_dir);

        } else if (is_dir && recursion > 0) {
            watch_list.append(QString::fromStdString(physical_name));
            AddFstEntriesToGameList(physical_name, recursion - 1, parent_dir);
        }

        return true;
    }};

    FileUtil::ForeachDirectoryEntry(nullptr, dir_path, callback);
}

void GameListWorker::run() {
    stop_processing = false;
    for (UISettings::GameDir& game_dir : game_dirs) {
        if (game_dir.path == "INSTALLED") {
            // Add normal titles
            {
                QString path{
                    QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir,
                                                                 Settings::values.sdmc_dir + "/") +
                                           "Nintendo "
                                           "3DS/00000000000000000000000000000000/"
                                           "00000000000000000000000000000000/title/00040000")};
                watch_list.append(path);
                GameListDir* game_list_dir{
                    new GameListDir(game_dir, GameListItemType::InstalledDir)};
                emit DirEntryReady({game_list_dir});
                AddFstEntriesToGameList(path.toStdString(), 2, game_list_dir);
            }
            // Add demos
            {
                QString path{
                    QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir,
                                                                 Settings::values.sdmc_dir + "/") +
                                           "Nintendo "
                                           "3DS/00000000000000000000000000000000/"
                                           "00000000000000000000000000000000/title/00040002")};
                watch_list.append(path);
                GameListDir* game_list_dir{
                    new GameListDir(game_dir, GameListItemType::InstalledDir)};
                emit DirEntryReady({game_list_dir});
                AddFstEntriesToGameList(path.toStdString(), 2, game_list_dir);
            }
        } else if (game_dir.path == "SYSTEM") {
            QString path{QString::fromStdString(FileUtil::GetUserPath(
                             FileUtil::UserPath::NANDDir, Settings::values.nand_dir + "/")) +
                         "00000000000000000000000000000000/title/00040010"};
            watch_list.append(path);
            GameListDir* game_list_dir{new GameListDir(game_dir, GameListItemType::SystemDir)};
            emit DirEntryReady({game_list_dir});
            AddFstEntriesToGameList(path.toStdString(), 2, game_list_dir);
        } else {
            watch_list.append(game_dir.path);
            GameListDir* game_list_dir{new GameListDir(game_dir)};
            emit DirEntryReady({game_list_dir});
            AddFstEntriesToGameList(game_dir.path.toStdString(), game_dir.deep_scan ? 256 : 0,
                                    game_list_dir);
        }
    };
    emit Finished(watch_list);
}

void GameListWorker::Cancel() {
    disconnect();
    stop_processing = true;
}
