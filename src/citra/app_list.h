// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QString>
#include <QWidget>
#include "common/common_types.h"
#include "ui_settings.h"

class AppListWorker;
class AppListDir;
class AppListSearchField;
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

enum class AppListOpenTarget { SaveData = 0, ExtData = 1, Application = 2, UpdateData = 3 };

class AppList : public QWidget {
    Q_OBJECT

public:
    enum {
        COLUMN_NAME,
        COLUMN_COMPATIBILITY,
        COLUMN_REGION,
        COLUMN_FILE_TYPE,
        COLUMN_SIZE,
        COLUMN_COUNT, // Number of columns
    };

    explicit AppList(GMainWindow* parent = nullptr);
    ~AppList() override;

    QString getLastFilterResultItem();
    void clearFilter();
    void setFilterFocus();
    void setFilterVisible(bool visibility);
    void setDirectoryWatcherEnabled(bool enabled);
    bool isEmpty();

    void PopulateAsync(QList<UISettings::AppDir>& app_dirs);

    void SaveInterfaceLayout();
    void LoadInterfaceLayout();

    QStandardItemModel* GetModel() const;

    QString FindApplicationByProgramID(u64 program_id);

    void Refresh();

    static const QStringList supported_file_extensions;

signals:
    void ApplicationChosen(QString app_path);
    void ShouldCancelWorker();
    void OpenFolderRequested(u64 program_id, AppListOpenTarget target);
    void OpenDirectory(QString directory);
    void AddDirectory();
    void ShowList(bool show);

private slots:
    void onItemExpanded(const QModelIndex& item);
    void onTextChanged(const QString& newText);
    void onFilterCloseClicked();
    void onUpdateThemedIcons();

private:
    void AddDirEntry(AppListDir* entry_items);
    void AddEntry(const QList<QStandardItem*>& entry_items, AppListDir* parent);
    void ValidateEntry(const QModelIndex& item);
    void DonePopulating(QStringList watch_list);

    void PopupContextMenu(const QPoint& menu_location);
    void AddAppPopup(QMenu& context_menu, QStandardItem* child);
    void AddCustomDirPopup(QMenu& context_menu, QStandardItem* child);
    void AddPermDirPopup(QMenu& context_menu, QStandardItem* child);

    QString FindApplicationByProgramID(QStandardItem* current_item, u64 program_id);

    AppListSearchField* search_field;
    GMainWindow* main_window;
    QVBoxLayout* layout;
    QTreeView* tree_view;
    QStandardItemModel* item_model;
    AppListWorker* current_worker;
    QFileSystemWatcher* watcher;

    friend class AppListSearchField;
};

Q_DECLARE_METATYPE(AppListOpenTarget);

class AppListPlaceholder : public QWidget {
    Q_OBJECT
public:
    explicit AppListPlaceholder(GMainWindow* parent = nullptr);
    ~AppListPlaceholder();

signals:
    void AddDirectory();

private slots:
    void onUpdateThemedIcons();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    GMainWindow* main_window = nullptr;
    QVBoxLayout* layout{};
    QLabel* image{};
    QLabel* text{};
};
