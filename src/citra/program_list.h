// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QString>
#include <QWidget>
#include "common/common_types.h"
#include "ui_settings.h"

namespace Core {
class System;
} // namespace Core

class ProgramListWorker;
class ProgramListDir;
class ProgramListSearchField;
class GMainWindow;
class QFileSystemWatcher;
class QHBoxLayout;
class QLabel;
class QLineEdit;
template <typename>
class QList;
class QModelIndex;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QToolButton;
class QVBoxLayout;
class QMenu;

enum class ProgramListOpenTarget { SaveData = 0, ExtData = 1, Program = 2, UpdateData = 3 };

class ProgramList : public QWidget {
    Q_OBJECT

public:
    enum {
        COLUMN_NAME,
        COLUMN_ISSUES,
        COLUMN_REGION,
        COLUMN_FILE_TYPE,
        COLUMN_SIZE,
        COLUMN_COUNT, // Number of columns
    };

    explicit ProgramList(Core::System& system, GMainWindow* parent = nullptr);
    ~ProgramList() override;

    QString getLastFilterResultItem();
    void clearFilter();
    void setFilterFocus();
    void setFilterVisible(bool visibility);
    void setDirectoryWatcherEnabled(bool enabled);
    bool isEmpty();

    void PopulateAsync(QList<UISettings::AppDir>& program_dirs);

    void SaveInterfaceLayout();
    void LoadInterfaceLayout();

    QStandardItemModel* GetModel() const;

    QString FindProgramByProgramID(u64 program_id);

    void Refresh();

    static const QStringList supported_file_extensions;

signals:
    void ProgramChosen(QString program_path);
    void ShouldCancelWorker();
    void OpenFolderRequested(u64 program_id, ProgramListOpenTarget target);
    void OpenDirectory(QString directory);
    void AddDirectory();
    void ShowList(bool show);

private slots:
    void OnItemExpanded(const QModelIndex& item);
    void onTextChanged(const QString& newText);
    void onFilterCloseClicked();
    void OnUpdateThemedIcons();

private:
    void AddDirEntry(ProgramListDir* entry_items);
    void AddEntry(const QList<QStandardItem*>& entry_items, ProgramListDir* parent);
    void ValidateEntry(const QModelIndex& item);
    void DonePopulating(QStringList watch_list);

    void PopupContextMenu(const QPoint& menu_location);
    void AddAppPopup(QMenu& context_menu, QStandardItem* child);
    void AddCustomDirPopup(QMenu& context_menu, QStandardItem* child);
    void AddPermDirPopup(QMenu& context_menu, QStandardItem* child);

    QString FindProgramByProgramID(QStandardItem* current_item, u64 program_id);

    ProgramListSearchField* search_field;
    GMainWindow* main_window;
    QVBoxLayout* layout;
    QTreeView* tree_view;
    QStandardItemModel* item_model;
    ProgramListWorker* current_worker;
    QFileSystemWatcher* watcher;
    Core::System& system;

    friend class ProgramListSearchField;
};

Q_DECLARE_METATYPE(ProgramListOpenTarget);

class ProgramListPlaceholder : public QWidget {
    Q_OBJECT
public:
    explicit ProgramListPlaceholder(GMainWindow* parent = nullptr);
    ~ProgramListPlaceholder();

signals:
    void AddDirectory();

private slots:
    void OnUpdateThemedIcons();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    GMainWindow* main_window = nullptr;
    QVBoxLayout* layout{};
    QLabel* image{};
    QLabel* text{};
};
