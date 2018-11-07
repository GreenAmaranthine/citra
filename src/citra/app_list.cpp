// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QThreadPool>
#include <QToolButton>
#include <QTreeView>
#include "citra/app_list.h"
#include "citra/app_list_p.h"
#include "citra/app_list_worker.h"
#include "citra/main.h"
#include "citra/ui_settings.h"
#include "common/common_paths.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/file_sys/archive_extsavedata.h"
#include "core/file_sys/archive_source_sd_savedata.h"
#include "core/hle/service/fs/archive.h"
#include "core/settings.h"

AppListSearchField::KeyReleaseEater::KeyReleaseEater(AppList* applist) : applist{applist} {}

// EventFilter in order to process systemkeys while editing the searchfield
bool AppListSearchField::KeyReleaseEater::eventFilter(QObject* obj, QEvent* event) {
    // If it isn't a KeyRelease event then continue with standard event processing
    if (event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(obj, event);

    QKeyEvent* keyEvent{static_cast<QKeyEvent*>(event)};
    QString edit_filter_text{applist->search_field->edit_filter->text().toLower()};

    // If the searchfield's text hasn't changed special function keys get checked
    // If no function key changes the searchfield's text the filter doesn't need to get reloaded
    if (edit_filter_text == edit_filter_text_old) {
        switch (keyEvent->key()) {
        // Escape: Resets the searchfield
        case Qt::Key_Escape: {
            if (edit_filter_text_old.isEmpty()) {
                return QObject::eventFilter(obj, event);
            } else {
                applist->search_field->edit_filter->clear();
                edit_filter_text.clear();
            }
            break;
        }
        // Return and Enter
        // If the enter key gets pressed first checks how many and which entry is visible
        // If there is only one result launch this application
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            if (applist->search_field->visible == 1) {
                QString file_path{applist->getLastFilterResultItem()};
                // To avoid loading error dialog loops while confirming them using enter
                // Also users usually want to run a different application after closing one
                applist->search_field->edit_filter->clear();
                edit_filter_text.clear();
                emit applist->ApplicationChosen(file_path);
            } else {
                return QObject::eventFilter(obj, event);
            }
            break;
        }
        default:
            return QObject::eventFilter(obj, event);
        }
    }
    edit_filter_text_old = edit_filter_text;
    return QObject::eventFilter(obj, event);
}

void AppListSearchField::setFilterResult(int visible, int total) {
    this->visible = visible;
    this->total = total;

    label_filter_result->setText(
        QString("%1 of %3 %4").arg(visible).arg(total).arg(total == 1 ? "result" : "results"));
}

QString AppList::getLastFilterResultItem() {
    QStandardItem* folder;
    QStandardItem* child;
    QString file_path;
    int folderCount{item_model->rowCount()};
    for (int i{}; i < folderCount; ++i) {
        folder = item_model->item(i, 0);
        QModelIndex folder_index{folder->index()};
        int childrenCount{folder->rowCount()};
        for (int j{}; j < childrenCount; ++j) {
            if (!tree_view->isRowHidden(j, folder_index)) {
                child = folder->child(j, 0);
                file_path = child->data(AppListItemPath::FullPathRole).toString();
            }
        }
    }
    return file_path;
}

void AppListSearchField::clear() {
    edit_filter->clear();
}

void AppListSearchField::setFocus() {
    if (edit_filter->isVisible())
        edit_filter->setFocus();
}

AppListSearchField::AppListSearchField(AppList* parent) : QWidget{parent} {
    KeyReleaseEater* keyReleaseEater{new KeyReleaseEater(parent)};
    layout_filter = new QHBoxLayout;
    layout_filter->setMargin(8);
    label_filter = new QLabel;
    label_filter->setText("Filter:");
    edit_filter = new QLineEdit;
    edit_filter->clear();
    edit_filter->setPlaceholderText("Enter pattern to filter");
    edit_filter->installEventFilter(keyReleaseEater);
    edit_filter->setClearButtonEnabled(true);
    connect(edit_filter, &QLineEdit::textChanged, parent, &AppList::onTextChanged);
    label_filter_result = new QLabel;
    button_filter_close = new QToolButton(this);
    button_filter_close->setText("X");
    button_filter_close->setCursor(Qt::ArrowCursor);
    button_filter_close->setStyleSheet("QToolButton{ border: none; padding: 0px; color: "
                                       "#000000; font-weight: bold; background: #F0F0F0; }"
                                       "QToolButton:hover{ border: none; padding: 0px; color: "
                                       "#EEEEEE; font-weight: bold; background: #E81123}");
    connect(button_filter_close, &QToolButton::clicked, parent, &AppList::onFilterCloseClicked);
    layout_filter->setSpacing(10);
    layout_filter->addWidget(label_filter);
    layout_filter->addWidget(edit_filter);
    layout_filter->addWidget(label_filter_result);
    layout_filter->addWidget(button_filter_close);
    setLayout(layout_filter);
}

/**
 * Checks if all words separated by spaces are contained in another string
 * This offers a word order insensitive search function
 *
 * @param String that gets checked if it contains all words of the userinput string
 * @param String containing all words getting checked
 * @return true if the haystack contains all words of userinput
 */

static bool ContainsAllWords(const QString& haystack, const QString& userinput) {
    const QStringList userinput_split{userinput.split(' ', QString::SplitBehavior::SkipEmptyParts)};

    return std::all_of(userinput_split.begin(), userinput_split.end(),
                       [&haystack](const QString& s) { return haystack.contains(s); });
}

// Syncs the expanded state of Application Directories with settings to persist across sessions
void AppList::OnItemExpanded(const QModelIndex& item) {
    // The click should still register in the AppListItemPath item no matter which column was
    // clicked
    int row{item_model->itemFromIndex(item)->row()};
    QStandardItem* child{item_model->invisibleRootItem()->child(row, COLUMN_NAME)};
    AppListItemType type{static_cast<AppListItemType>(child->type())};
    if (type == AppListItemType::CustomDir || type == AppListItemType::InstalledDir ||
        type == AppListItemType::SystemDir)
        child->data(AppListDir::AppDirRole).value<UISettings::AppDir*>()->expanded =
            tree_view->isExpanded(child->index());
}

// Event in order to filter the applist after editing the searchfield
void AppList::onTextChanged(const QString& newText) {
    int folderCount{tree_view->model()->rowCount()};
    QString edit_filter_text{newText.toLower()};
    QStandardItem* folder;
    int childrenTotal{};

    // If the searchfield is empty every item is visible
    // Otherwise the filter gets applied
    if (edit_filter_text.isEmpty()) {
        for (int i{}; i < folderCount; ++i) {
            folder = item_model->item(i, 0);
            QModelIndex folder_index{folder->index()};
            int childrenCount{folder->rowCount()};
            for (int j{}; j < childrenCount; ++j) {
                ++childrenTotal;
                tree_view->setRowHidden(j, folder_index, false);
            }
        }
        search_field->setFilterResult(childrenTotal, childrenTotal);
    } else {
        int result_count{};
        for (int i{}; i < folderCount; ++i) {
            folder = item_model->item(i, 0);
            auto folder_index{folder->index()};
            int childrenCount{folder->rowCount()};
            for (int j{}; j < childrenCount; ++j) {
                ++childrenTotal;
                const auto child{folder->child(j, 0)};
                const auto file_path{
                    child->data(AppListItemPath::FullPathRole).toString().toLower()};
                auto file_name{file_path.mid(file_path.lastIndexOf("/") + 1)};
                auto file_title{child->data(AppListItemPath::TitleRole).toString().toLower()};
                auto file_programid{
                    child->data(AppListItemPath::ProgramIdRole).toString().toLower()};
                // Only items which filename in combination with its title contains all words
                // that are in the searchfield will be visible in the applist
                // The search is case insensitive because of toLower()
                // I decided not to use Qt::CaseInsensitive in ContainsAllWords to prevent
                // multiple conversions of edit_filter_text for each application in the application
                // list
                if (ContainsAllWords(file_name.append(' ').append(file_title), edit_filter_text) ||
                    (file_programid.count() == 16 && edit_filter_text.contains(file_programid))) {
                    tree_view->setRowHidden(j, folder_index, false);
                    ++result_count;
                } else
                    tree_view->setRowHidden(j, folder_index, true);
                search_field->setFilterResult(result_count, childrenTotal);
            }
        }
    }
}

void AppList::OnUpdateThemedIcons() {
    for (int i{}; i < item_model->invisibleRootItem()->rowCount(); i++) {
        QStandardItem* child{item_model->invisibleRootItem()->child(i)};
        switch (static_cast<AppListItemType>(child->type())) {
        case AppListItemType::InstalledDir:
            child->setData(QIcon::fromTheme("sd_card").pixmap(48), Qt::DecorationRole);
            break;
        case AppListItemType::SystemDir:
            child->setData(QIcon::fromTheme("chip").pixmap(48), Qt::DecorationRole);
            break;
        case AppListItemType::CustomDir: {
            const UISettings::AppDir* app_dir{
                child->data(AppListDir::AppDirRole).value<UISettings::AppDir*>()};
            QString icon_name{QFileInfo::exists(app_dir->path) ? "folder" : "bad_folder"};
            child->setData(QIcon::fromTheme(icon_name).pixmap(48), Qt::DecorationRole);
            break;
        }
        case AppListItemType::AddDir:
            child->setData(QIcon::fromTheme("plus").pixmap(48), Qt::DecorationRole);
            break;
        }
    }
}

void AppList::onFilterCloseClicked() {
    main_window->filterBarSetChecked(false);
}

AppList::AppList(Core::System& system, GMainWindow* parent) : QWidget{parent}, system{system} {
    watcher = new QFileSystemWatcher(this);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &AppList::Refresh,
            Qt::UniqueConnection);
    this->main_window = parent;
    layout = new QVBoxLayout;
    tree_view = new QTreeView;
    search_field = new AppListSearchField(this);
    item_model = new QStandardItemModel(tree_view);
    tree_view->setModel(item_model);
    tree_view->setAlternatingRowColors(true);
    tree_view->setSelectionMode(QHeaderView::SingleSelection);
    tree_view->setSelectionBehavior(QHeaderView::SelectRows);
    tree_view->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    tree_view->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    tree_view->setSortingEnabled(true);
    tree_view->setEditTriggers(QHeaderView::NoEditTriggers);
    tree_view->setUniformRowHeights(true);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    item_model->insertColumns(0, COLUMN_COUNT);
    item_model->setHeaderData(COLUMN_NAME, Qt::Horizontal, "Name");
    item_model->setHeaderData(COLUMN_COMPATIBILITY, Qt::Horizontal, "Compatibility");
    item_model->setHeaderData(COLUMN_REGION, Qt::Horizontal, "Region");
    item_model->setHeaderData(COLUMN_FILE_TYPE, Qt::Horizontal, "File type");
    item_model->setHeaderData(COLUMN_SIZE, Qt::Horizontal, "Size");
    tree_view->setColumnWidth(COLUMN_NAME, 500);
    tree_view->setColumnWidth(COLUMN_COMPATIBILITY, 115);
    item_model->setSortRole(AppListItemPath::TitleRole);
    connect(main_window, &GMainWindow::UpdateThemedIcons, this, &AppList::OnUpdateThemedIcons);
    connect(tree_view, &QTreeView::activated, this, &AppList::ValidateEntry);
    connect(tree_view, &QTreeView::customContextMenuRequested, this, &AppList::PopupContextMenu);
    connect(tree_view, &QTreeView::expanded, this, &AppList::OnItemExpanded);
    connect(tree_view, &QTreeView::collapsed, this, &AppList::OnItemExpanded);
    // We must register all custom types with the Qt Automoc system so that we're able to use
    // it with signals/slots. In this case, QList falls under the umbrells of custom types.
    qRegisterMetaType<QList<QStandardItem*>>("QList<QStandardItem*>");
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tree_view);
    layout->addWidget(search_field);
    setLayout(layout);
}

AppList::~AppList() {
    emit ShouldCancelWorker();
}

void AppList::setFilterFocus() {
    if (tree_view->model()->rowCount() > 0)
        search_field->setFocus();
}

void AppList::setFilterVisible(bool visibility) {
    search_field->setVisible(visibility);
}

void AppList::setDirectoryWatcherEnabled(bool enabled) {
    if (enabled)
        connect(watcher, &QFileSystemWatcher::directoryChanged, this, &AppList::Refresh,
                Qt::UniqueConnection);
    else
        disconnect(watcher, &QFileSystemWatcher::directoryChanged, this, &AppList::Refresh);
}

void AppList::clearFilter() {
    search_field->clear();
}

void AppList::AddDirEntry(AppListDir* entry_items) {
    item_model->invisibleRootItem()->appendRow(entry_items);
    tree_view->setExpanded(
        entry_items->index(),
        entry_items->data(AppListDir::AppDirRole).value<UISettings::AppDir*>()->expanded);
}

void AppList::AddEntry(const QList<QStandardItem*>& entry_items, AppListDir* parent) {
    parent->appendRow(entry_items);
}

void AppList::ValidateEntry(const QModelIndex& item) {
    // The click should still register in the first column no matter which column was
    // clicked
    int row{item_model->itemFromIndex(item)->row()};
    const QStandardItem* parent{item_model->itemFromIndex(item.parent())};
    if (!parent)
        parent = item_model->invisibleRootItem();
    const QStandardItem* child{parent->child(row, COLUMN_NAME)};

    switch (static_cast<AppListItemType>(child->type())) {
    case AppListItemType::Application: {
        QString file_path{child->data(AppListItemPath::FullPathRole).toString()};
        if (file_path.isEmpty())
            return;
        QFileInfo file_info{file_path};
        if (!file_info.exists() || file_info.isDir())
            return;
        // Users usually want to run a different application after closing one
        search_field->clear();
        emit ApplicationChosen(file_path);
        break;
    }
    case AppListItemType::AddDir:
        emit AddDirectory();
        break;
    }
}

bool AppList::isEmpty() {
    for (int i{}; i < item_model->invisibleRootItem()->rowCount(); i++) {
        const QStandardItem* child{item_model->invisibleRootItem()->child(i)};
        AppListItemType type{static_cast<AppListItemType>(child->type())};
        if (!child->hasChildren() &&
            (type == AppListItemType::InstalledDir || type == AppListItemType::SystemDir)) {
            item_model->invisibleRootItem()->removeRow(child->row());
            i--;
        };
    }
    return !item_model->invisibleRootItem()->hasChildren();
}

void AppList::DonePopulating(QStringList watch_list) {
    if (isEmpty())
        emit ShowList(false);
    else {
        item_model->sort(COLUMN_NAME);
        item_model->invisibleRootItem()->appendRow(new AppListAddDir());
        emit ShowList(true);
    }
    // Clear out the old directories to watch for changes and add the new ones
    auto watch_dirs{watcher->directories()};
    if (!watch_dirs.isEmpty())
        watcher->removePaths(watch_dirs);
    // Workaround: Add the watch paths in chunks to allow the gui to refresh
    // This prevents the UI from stalling when a large number of watch paths are added
    // Also artificially caps the watcher to a certain number of directories
    constexpr int LIMIT_WATCH_DIRECTORIES{5000};
    constexpr int SLICE_SIZE{25};
    int len{std::min(watch_list.length(), LIMIT_WATCH_DIRECTORIES)};
    for (int i{}; i < len; i += SLICE_SIZE) {
        watcher->addPaths(watch_list.mid(i, i + SLICE_SIZE));
        QCoreApplication::processEvents();
    }
    tree_view->setEnabled(true);
    int folderCount{tree_view->model()->rowCount()};
    int childrenTotal{};
    for (int i{}; i < folderCount; ++i) {
        int childrenCount{item_model->item(i, 0)->rowCount()};
        for (int j{}; j < childrenCount; ++j)
            ++childrenTotal;
    }
    search_field->setFilterResult(childrenTotal, childrenTotal);
    if (childrenTotal > 0)
        search_field->setFocus();
}

void AppList::PopupContextMenu(const QPoint& menu_location) {
    QModelIndex item{tree_view->indexAt(menu_location)};
    if (!item.isValid())
        return;
    // The click should still register in the first column no matter which column was
    // clicked
    int row{item_model->itemFromIndex(item)->row()};
    QStandardItem* parent{item_model->itemFromIndex(item.parent())};
    // invisibleRootItem() will not appear as an item's parent
    if (!parent)
        parent = item_model->invisibleRootItem();
    QStandardItem* child{parent->child(row, COLUMN_NAME)};
    QMenu context_menu;
    switch (static_cast<AppListItemType>(child->type())) {
    case AppListItemType::Application:
        AddAppPopup(context_menu, child);
        break;
    case AppListItemType::CustomDir:
        AddPermDirPopup(context_menu, child);
        AddCustomDirPopup(context_menu, child);
        break;
    case AppListItemType::InstalledDir:
    case AppListItemType::SystemDir:
        AddPermDirPopup(context_menu, child);
        break;
    }
    context_menu.exec(tree_view->viewport()->mapToGlobal(menu_location));
}

void AppList::AddAppPopup(QMenu& context_menu, QStandardItem* child) {
    u64 program_id{child->data(AppListItemPath::ProgramIdRole).toULongLong()};
    u64 extdata_id{child->data(AppListItemPath::ExtdataIdRole).toULongLong()};
    QString path{child->data(AppListItemPath::FullPathRole).toString()};
    QAction* open_save_location{context_menu.addAction("Open Save Data Location")};
    QAction* open_extdata_location{context_menu.addAction("Open Extra Data Location")};
    QAction* open_application_location{context_menu.addAction("Open Application Location")};
    QAction* open_update_location{context_menu.addAction("Open Update Data Location")};
    QAction* copy_program_id{context_menu.addAction("Copy Program ID")};
    open_save_location->setVisible(0x0004000000000000 <= program_id &&
                                   program_id <= 0x00040000FFFFFFFF);
    std::string sdmc_dir{
        FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir, Settings::values.sdmc_dir + "/")};
    open_save_location->setEnabled(FileUtil::Exists(
        FileSys::ArchiveSource_SDSaveData::GetSaveDataPathFor(sdmc_dir, program_id)));
    if (extdata_id)
        open_extdata_location->setVisible(
            0x0004000000000000 <= program_id && program_id <= 0x00040000FFFFFFFF &&
            FileUtil::Exists(FileSys::GetExtDataPathFromId(sdmc_dir, extdata_id)));
    else
        open_extdata_location->setVisible(false);
    auto media_type{Service::AM::GetTitleMediaType(program_id)};
    open_application_location->setVisible(path.toStdString() ==
                                          Service::AM::GetTitleContentPath(media_type, program_id));
    open_update_location->setVisible(
        0x0004000000000000 <= program_id && program_id <= 0x00040000FFFFFFFF &&
        FileUtil::Exists(
            Service::AM::GetTitlePath(Service::FS::MediaType::SDMC, program_id + 0xe00000000) +
            "content/"));
    connect(open_save_location, &QAction::triggered, [&, program_id]() {
        emit OpenFolderRequested(program_id, AppListOpenTarget::SaveData);
    });
    connect(open_extdata_location, &QAction::triggered, [this, extdata_id] {
        emit OpenFolderRequested(extdata_id, AppListOpenTarget::ExtData);
    });
    connect(open_application_location, &QAction::triggered, [&, program_id]() {
        emit OpenFolderRequested(program_id, AppListOpenTarget::Application);
    });
    connect(open_update_location, &QAction::triggered, [&, program_id]() {
        emit OpenFolderRequested(program_id, AppListOpenTarget::UpdateData);
    });
    connect(copy_program_id, &QAction::triggered, [&, program_id]() {
        QApplication::clipboard()->setText(
            QString::fromStdString(fmt::format("{:016X}", program_id)));
    });
    auto it{compatibility_database.find(program_id)};
    if (it != compatibility_database.end() && it->second.size() != 0) {
        QAction* issues{context_menu.addAction("Issues")};
        connect(issues, &QAction::triggered, [&, list = it->second]() {
            QMessageBox::information(this, "Issues", list.join("<br>"));
        });
    }
};

void AppList::AddCustomDirPopup(QMenu& context_menu, QStandardItem* child) {
    UISettings::AppDir& app_dir{*child->data(AppListDir::AppDirRole).value<UISettings::AppDir*>()};
    QAction* deep_scan{context_menu.addAction("Scan Subfolders")};
    QAction* delete_dir{context_menu.addAction("Remove Application Directory")};
    deep_scan->setCheckable(true);
    deep_scan->setChecked(app_dir.deep_scan);
    connect(deep_scan, &QAction::triggered, [&] {
        app_dir.deep_scan = !app_dir.deep_scan;
        PopulateAsync(UISettings::values.app_dirs);
    });
    connect(delete_dir, &QAction::triggered, [&, child] {
        UISettings::values.app_dirs.removeOne(app_dir);
        item_model->invisibleRootItem()->removeRow(child->row());
    });
}

void AppList::AddPermDirPopup(QMenu& context_menu, QStandardItem* child) {
    UISettings::AppDir& app_dir{*child->data(AppListDir::AppDirRole).value<UISettings::AppDir*>()};
    QAction* move_up{context_menu.addAction("\U000025b2 Move Up")};
    QAction* move_down{context_menu.addAction("\U000025bc Move Down")};
    QAction* open_directory_location{context_menu.addAction("Open Directory Location")};
    int row{child->row()};
    move_up->setEnabled(row > 0);
    move_down->setEnabled(row < item_model->invisibleRootItem()->rowCount() - 2);
    connect(move_up, &QAction::triggered, [&, child, row] {
        // Find the indices of the items in settings and swap them
        UISettings::values.app_dirs.swap(
            UISettings::values.app_dirs.indexOf(app_dir),
            UISettings::values.app_dirs.indexOf(*item_model->invisibleRootItem()
                                                     ->child(row - 1, COLUMN_NAME)
                                                     ->data(AppListDir::AppDirRole)
                                                     .value<UISettings::AppDir*>()));
        // Move the treeview items
        QList<QStandardItem*> item = item_model->takeRow(row);
        item_model->invisibleRootItem()->insertRow(row - 1, item);
        tree_view->setExpanded(child->index(), app_dir.expanded);
    });
    connect(move_down, &QAction::triggered, [&, child, row] {
        // Find the indices of the items in settings and swap them
        UISettings::values.app_dirs.swap(
            UISettings::values.app_dirs.indexOf(
                *child->data(AppListDir::AppDirRole).value<UISettings::AppDir*>()),
            UISettings::values.app_dirs.indexOf(*item_model->invisibleRootItem()
                                                     ->child(row + 1, COLUMN_NAME)
                                                     ->data(AppListDir::AppDirRole)
                                                     .value<UISettings::AppDir*>()));
        // Move the treeview items
        QList<QStandardItem*> item{item_model->takeRow(row)};
        item_model->invisibleRootItem()->insertRow(row + 1, item);
        tree_view->setExpanded(child->index(), app_dir.expanded);
    });
    connect(open_directory_location, &QAction::triggered,
            [&] { emit OpenDirectory(app_dir.path); });
}

QStandardItemModel* AppList::GetModel() const {
    return item_model;
}

void AppList::PopulateAsync(QList<UISettings::AppDir>& app_dirs) {
    tree_view->setEnabled(false);
    // Delete any rows that might already exist if we're repopulating
    item_model->removeRows(0, item_model->rowCount());
    search_field->clear();
    emit ShouldCancelWorker();
    auto worker{new AppListWorker(system, app_dirs)};
    connect(worker, &AppListWorker::EntryReady, this, &AppList::AddEntry, Qt::QueuedConnection);
    connect(worker, &AppListWorker::DirEntryReady, this, &AppList::AddDirEntry,
            Qt::QueuedConnection);
    connect(worker, &AppListWorker::Finished, this, &AppList::DonePopulating, Qt::QueuedConnection);
    // Use DirectConnection here because worker->Cancel() is thread-safe and we want it to
    // cancel without delay.
    connect(this, &AppList::ShouldCancelWorker, worker, &AppListWorker::Cancel,
            Qt::DirectConnection);
    QThreadPool::globalInstance()->start(worker);
    current_worker = std::move(worker);
}

void AppList::SaveInterfaceLayout() {
    UISettings::values.applist_header_state = tree_view->header()->saveState();
}

void AppList::LoadInterfaceLayout() {
    auto header{tree_view->header()};
    if (!header->restoreState(UISettings::values.applist_header_state))
        // We're using the name column to display icons and titles
        // so make it as large as possible as default.
        header->resizeSection(COLUMN_NAME, header->width());
    item_model->sort(header->sortIndicatorSection(), header->sortIndicatorOrder());
}

const QStringList AppList::supported_file_extensions{
    {"3ds", "3dsx", "elf", "axf", "cci", "cxi", "app"}};

void AppList::Refresh() {
    if (!UISettings::values.app_dirs.isEmpty() && current_worker) {
        LOG_INFO(Frontend,
                 "Change detected in the application directories. Reloading application list.");
        PopulateAsync(UISettings::values.app_dirs);
    }
}

QString AppList::FindApplicationByProgramID(u64 program_id) {
    return FindApplicationByProgramID(item_model->invisibleRootItem(), program_id);
}

QString AppList::FindApplicationByProgramID(QStandardItem* current_item, u64 program_id) {
    if (current_item->type() == static_cast<int>(AppListItemType::Application) &&
        current_item->data(AppListItemPath::ProgramIdRole).toULongLong() == program_id) {
        return current_item->data(AppListItemPath::FullPathRole).toString();
    } else if (current_item->hasChildren()) {
        for (int child_id{}; child_id < current_item->rowCount(); child_id++) {
            QString path{FindApplicationByProgramID(current_item->child(child_id, 0), program_id)};
            if (!path.isEmpty())
                return path;
        }
    }
    return "";
}

AppListPlaceholder::AppListPlaceholder(GMainWindow* parent) : QWidget{parent}, main_window{parent} {
    main_window = parent;
    connect(main_window, &GMainWindow::UpdateThemedIcons, this,
            &AppListPlaceholder::OnUpdateThemedIcons);
    layout = new QVBoxLayout;
    image = new QLabel;
    text = new QLabel;
    layout->setAlignment(Qt::AlignCenter);
    image->setPixmap(QIcon::fromTheme("plus_folder").pixmap(200));
    text->setText("Double-click to add a new folder to the application list ");
    QFont font = text->font();
    font.setPointSize(20);
    text->setFont(font);
    text->setAlignment(Qt::AlignHCenter);
    image->setAlignment(Qt::AlignHCenter);
    layout->addWidget(image);
    layout->addWidget(text);
    setLayout(layout);
}

AppListPlaceholder::~AppListPlaceholder() = default;

void AppListPlaceholder::OnUpdateThemedIcons() {
    image->setPixmap(QIcon::fromTheme("plus_folder").pixmap(200));
}

void AppListPlaceholder::mouseDoubleClickEvent(QMouseEvent* event) {
    emit AppListPlaceholder::AddDirectory();
}
