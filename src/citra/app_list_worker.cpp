// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <QFileInfo>
#include "citra/app_list.h"
#include "citra/app_list_p.h"
#include "citra/app_list_worker.h"
#include "citra/ui_settings.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/fs/archive.h"
#include "core/loader/loader.h"

namespace {

static bool HasSupportedFileExtension(const std::string& file_name) {
    QFileInfo file{QString::fromStdString(file_name)};
    return AppList::supported_file_extensions.contains(file.suffix(), Qt::CaseInsensitive);
}

} // Anonymous namespace

void AppListWorker::AddFstEntriesToAppList(const std::string& dir_path, unsigned int recursion,
                                           AppListDir* parent_dir) {
    const auto callback{[this, recursion, parent_dir](u64* num_entries_out,
                                                      const std::string& directory,
                                                      const std::string& virtual_name) -> bool {
        auto physical_name{fmt::format("{}/{}", directory, virtual_name)};
        if (stop_processing)
            return false; // Breaks the callback loop.
        bool is_dir{FileUtil::IsDirectory(physical_name)};
        if (!is_dir && HasSupportedFileExtension(physical_name)) {
            auto loader{Loader::GetLoader(system, physical_name)};
            if (!loader)
                return true;
            u64 program_id;
            loader->ReadProgramId(program_id);
            u64 extdata_id;
            loader->ReadExtdataId(extdata_id);
            auto smdh{[this, program_id, &loader]() -> std::vector<u8> {
                std::vector<u8> original_smdh;
                loader->ReadIcon(original_smdh);
                if (program_id < 0x0004000000000000 || program_id > 0x00040000FFFFFFFF)
                    return original_smdh;
                auto update_path{Service::AM::GetTitleContentPath(Service::FS::MediaType::SDMC,
                                                                  program_id + 0x0000000E00000000)};
                if (!FileUtil::Exists(update_path))
                    return original_smdh;
                auto update_loader{Loader::GetLoader(system, update_path)};
                if (!update_loader)
                    return original_smdh;
                std::vector<u8> update_smdh;
                update_loader->ReadIcon(update_smdh);
                return update_smdh;
            }()};
            if (!Loader::IsValidSMDH(smdh) && UISettings::values.app_list_hide_no_icon)
                // Skip this invalid entry
                return true;
            emit EntryReady(
                {
                    new AppListItemPath(QString::fromStdString(physical_name), smdh, program_id,
                                        extdata_id),
                    new AppListItemCompat(program_id),
                    new AppListItemRegion(smdh),
                    new AppListItem(
                        QString::fromStdString(Loader::GetFileTypeString(loader->GetFileType()))),
                    new AppListItemSize(FileUtil::GetSize(physical_name)),
                },
                parent_dir);
        } else if (is_dir && recursion > 0) {
            watch_list.append(QString::fromStdString(physical_name));
            AddFstEntriesToAppList(physical_name, recursion - 1, parent_dir);
        }
        return true;
    }};
    FileUtil::ForeachDirectoryEntry(nullptr, dir_path, callback);
}

void AppListWorker::run() {
    stop_processing = false;
    for (auto& app_dir : app_dirs) {
        if (app_dir.path == "INSTALLED") {
            // Add normal titles
            {
                auto path{
                    QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir,
                                                                 Settings::values.sdmc_dir + "/") +
                                           "Nintendo "
                                           "3DS/00000000000000000000000000000000/"
                                           "00000000000000000000000000000000/title/00040000")};
                watch_list.append(path);
                auto app_list_dir{new AppListDir(app_dir, AppListItemType::InstalledDir)};
                emit DirEntryReady({app_list_dir});
                AddFstEntriesToAppList(path.toStdString(), 2, app_list_dir);
            }
            // Add demos
            {
                auto path{
                    QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir,
                                                                 Settings::values.sdmc_dir + "/") +
                                           "Nintendo "
                                           "3DS/00000000000000000000000000000000/"
                                           "00000000000000000000000000000000/title/00040002")};
                watch_list.append(path);
                auto app_list_dir{new AppListDir(app_dir, AppListItemType::InstalledDir)};
                emit DirEntryReady({app_list_dir});
                AddFstEntriesToAppList(path.toStdString(), 2, app_list_dir);
            }
        } else if (app_dir.path == "SYSTEM") {
            auto path{QString::fromStdString(FileUtil::GetUserPath(
                          FileUtil::UserPath::NANDDir, Settings::values.nand_dir + "/")) +
                      "00000000000000000000000000000000/title/00040010"};
            watch_list.append(path);
            auto app_list_dir{new AppListDir(app_dir, AppListItemType::SystemDir)};
            emit DirEntryReady({app_list_dir});
            AddFstEntriesToAppList(path.toStdString(), 2, app_list_dir);
        } else {
            watch_list.append(app_dir.path);
            auto app_list_dir{new AppListDir(app_dir)};
            emit DirEntryReady({app_list_dir});
            AddFstEntriesToAppList(app_dir.path.toStdString(), app_dir.deep_scan ? 256 : 0,
                                   app_list_dir);
        }
    }
    emit Finished(watch_list);
}

void AppListWorker::Cancel() {
    disconnect();
    stop_processing = true;
}
