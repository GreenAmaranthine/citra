// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <string>
#include <QList>
#include <QObject>
#include <QRunnable>
#include <QString>
#include "common/common_types.h"

namespace Core {
class System;
} // namespace Core

/**
 * Asynchronous worker object for populating the program list.
 * Communicates with other threads through Qt's signal/slot system.
 */
class ProgramListWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit ProgramListWorker(Core::System& system, QList<UISettings::AppDir>& program_dirs)
        : QObject{}, QRunnable{}, program_dirs{program_dirs}, system{system} {}

public slots:
    /// Starts the processing of directory tree information.
    void run() override;

    /// Tells the worker that it should no longer continue processing. Thread-safe.
    void Cancel();

signals:
    /**
     * The `EntryReady` signal is emitted once an entry has been prepared and is ready
     * to be added to the program list.
     * @param entry_items a list with `QStandardItem`s that make up the columns of the new
     * entry.
     */
    void DirEntryReady(ProgramListDir* entry_items);
    void EntryReady(QList<QStandardItem*> entry_items, ProgramListDir* parent_dir);

    /**
     * After the worker has traversed the program directory looking for entries, this signal is
     * emitted with a list of folders that should be watched for changes as well.
     */
    void Finished(QStringList watch_list);

private:
    void AddFstEntriesToProgramList(const std::string& dir_path, unsigned int recursion,
                                    ProgramListDir* parent_dir);

    QStringList watch_list;
    QList<UISettings::AppDir>& program_dirs;
    std::atomic_bool stop_processing{};
    Core::System& system;
};
