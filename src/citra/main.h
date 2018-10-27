// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include "common/announce_multiplayer_room.h"
#include "core/core.h"
#include "core/hle/applets/erreula.h"
#include "core/hle/applets/mii_selector.h"
#include "core/hle/applets/swkbd.h"
#include "core/hle/service/am/am.h"
#include "ui_main.h"

class AboutDialog;
class CheatDialog;
class Config;
class ControlPanel;
class ClickableLabel;
class EmuThread;
class GameList;
enum class GameListOpenTarget;
class GameListPlaceholder;
class GImageInfo;
class Screens;
class MultiplayerState;
template <typename>
class QFutureWatcher;
class QProgressBar;

class GMainWindow : public QMainWindow {
    Q_OBJECT

    /// Max number of recently loaded items to keep track of
    static const int max_recent_files_item{10};

public:
    void filterBarSetChecked(bool state);
    void UpdateUITheme();

    GMainWindow();
    ~GMainWindow();

signals:
    /**
     * Signal that is emitted when a new EmuThread has been created and an emulation session is
     * about to start. At this time, the core system emulation has been initialized, and all
     * emulation handles and memory should be valid.
     *
     * @param emu_thread Pointer to the newly created EmuThread (to be used by widgets that need to
     *      access/change emulation state).
     */
    void EmulationStarting(EmuThread* emu_thread);

    /**
     * Signal that is emitted when emulation is about to stop. At this time, the EmuThread and core
     * system emulation handles and memory are still valid, but are about become invalid.
     */
    void EmulationStopping();

    void UpdateProgress(std::size_t written, std::size_t total);
    void CIAInstallReport(Service::AM::InstallStatus status, const QString& filepath);
    void CIAInstallFinished();

    // Signal that tells widgets to update icons to use the current theme
    void UpdateThemedIcons();

private:
    void InitializeWidgets();
    void InitializeRecentFileMenuActions();
    void InitializeHotkeys();
    void SetDefaultUIGeometry();

    void SyncMenuUISettings();
    void RestoreUIState();

    void ConnectWidgetEvents();
    void ConnectMenuEvents();

    bool LoadROM(const std::string& filename);
    void BootGame(const std::string& filename);
    void ShutdownGame();

    /**
     * Stores the filename in the recently loaded files list.
     * The new filename is stored at the beginning of the recently loaded files list.
     * After inserting the new entry, duplicates are removed meaning that if
     * this was inserted from OnMenuRecentFile(), the entry will be put on top
     * and remove from its previous position.
     *
     * Finally, this function calls \a UpdateRecentFiles() to update the UI.
     *
     * @param filename the filename to store
     */
    void StoreRecentFile(const QString& filename);

    /**
     * Updates the recent files menu.
     * Menu entries are rebuilt from the configuration file.
     * If there is no entry in the menu, the menu is greyed out.
     */
    void UpdateRecentFiles();

    /**
     * If the emulation is running,
     * asks the user if he really want to close the emulator
     *
     * @return true if the user confirmed
     */
    bool ConfirmClose();
    bool ConfirmChangeGame();
    void closeEvent(QCloseEvent* event) override;

private slots:
    void OnStartGame();
    void OnPauseGame();
    void OnStopGame();

    /// Called when user selects a game in the game list widget.
    void OnGameListLoadFile(const QString& path);

    void OnGameListOpenFolder(u64 program_id, GameListOpenTarget target);
    void OnGameListOpenDirectory(const QString& path);
    void OnGameListAddDirectory();
    void OnGameListShowList(bool show);
    void OnMenuLoadFile();
    void OnMenuInstallCIA();
    void OnMenuAddSeed();
    void OnUpdateProgress(std::size_t written, std::size_t total);
    void OnCIAInstallReport(Service::AM::InstallStatus status, const QString& filepath);
    void OnCIAInstallFinished();
    void OnMenuRecentFile();
    void OnOpenConfiguration();
    void OnSetPlayCoins();
    void OnCheats();
    void OnControlPanel();
    void OnOpenUserDirectory();
    void OnNANDDefault();
    void OnNANDCustom();
    void OnSDMCDefault();
    void OnSDMCCustom();
    void OnLoadAmiibo();
    void OnRemoveAmiibo();
    void OnToggleFilterBar();
    void ToggleFullscreen();
    void ChangeScreenLayout();
    void ToggleScreenLayout();
    void OnSwapScreens();
    void ToggleSleepMode();
    void ShowFullscreen();
    void HideFullscreen();
    void OnRecordMovie();
    void OnPlayMovie();
    void OnStopRecordingPlayback();
    void OnCaptureScreenshot();
    void OnCoreError(Core::System::ResultStatus, const std::string&);

    /// Called when user selects Help -> About Citra
    void OnMenuAboutCitra();

private:
    bool ValidateMovie(const QString& path, u64 program_id = 0);
    void UpdateStatusBar();
    void SetupUIStrings();

    Q_INVOKABLE void ErrEulaCallback(HLE::Applets::ErrEulaConfig& config, bool& open);
    Q_INVOKABLE void SwkbdCallback(HLE::Applets::SoftwareKeyboardConfig& config,
                                   std::u16string& text, bool& open);
    Q_INVOKABLE void MiiSelectorCallback(const HLE::Applets::MiiConfig& config,
                                         HLE::Applets::MiiResult& result, bool& open);
    Q_INVOKABLE void Update3D();
    Q_INVOKABLE void UpdateFrameAdvancingCallback();
    Q_INVOKABLE void UpdateControlPanelNetwork();
    Q_INVOKABLE void OnMoviePlaybackCompleted();

    Ui::MainWindow ui;

    Screens* screens;

    GameList* game_list;
    GameListPlaceholder* game_list_placeholder;

    // Status bar elements
    QProgressBar* progress_bar;
    QLabel* message_label;
    QLabel* perf_label;

    QTimer status_bar_update_timer;
    QTimer movie_play_timer;

    MultiplayerState* multiplayer_state;
    std::unique_ptr<Config> config;

    std::unique_ptr<EmuThread> emu_thread;

    // The short title of the game currently running
    std::string short_title;

    // Movie
    bool movie_record_on_start{};
    QString movie_record_path;

    std::shared_ptr<ControlPanel> control_panel;
    std::shared_ptr<CheatDialog> cheats_window;

    QAction* actions_recent_files[max_recent_files_item];

    // Stores default icon theme search paths for the platform
    QStringList default_theme_paths;

    Core::System& system{Core::System::GetInstance()};

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
};

Q_DECLARE_METATYPE(std::size_t);
Q_DECLARE_METATYPE(Service::AM::InstallStatus);
