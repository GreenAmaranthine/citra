// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef _MSC_VER
#include <intrin.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <clocale>
#include <memory>
#include <thread>
#include <glad/glad.h>
#define QT_NO_OPENGL
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <QtGui>
#include <QtWidgets>
#include <SDL.h>
#ifdef ENABLE_DISCORD_RPC
#include <discord_rpc.h>
#endif
#include <fmt/format.h>
#include "citra/aboutdialog.h"
#include "citra/bootmanager.h"
#include "citra/camera/qt_multimedia_camera.h"
#include "citra/camera/still_image_camera.h"
#include "citra/cheats.h"
#include "citra/configuration/config.h"
#include "citra/configuration/configure_dialog.h"
#include "citra/control_panel.h"
#include "citra/game_list.h"
#include "citra/hotkeys.h"
#include "citra/main.h"
#include "citra/mii_selector.h"
#include "citra/multiplayer/state.h"
#include "citra/swkbd.h"
#include "citra/ui_settings.h"
#include "citra/util/clickable_label.h"
#include "citra/util/console.h"
#include "common/common_paths.h"
#include "common/detached_tasks.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "common/logging/text_formatter.h"
#include "common/scm_rev.h"
#include "common/scope_exit.h"
#include "core/3ds.h"
#include "core/core.h"
#include "core/file_sys/archive_extsavedata.h"
#include "core/file_sys/archive_source_sd_savedata.h"
#include "core/file_sys/seed_db.h"
#include "core/hle/service/am/am_u.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/hle/service/fs/archive.h"
#include "core/hle/service/nwm/nwm_ext.h"
#include "core/hle/service/ptm/ptm.h"
#include "core/loader/loader.h"
#include "core/movie.h"
#include "core/rpc/rpc_server.h"
#include "core/settings.h"
#include "video_core/renderer/renderer.h"
#include "video_core/video_core.h"

#ifdef QT_STATICPLUGIN
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif

#ifdef _WIN32
extern "C" {
// tells Nvidia drivers to use the dedicated GPU by default on laptops with switchable graphics
__declspec(dllexport) unsigned long NvOptimusEnablement{0x00000001};
}
#endif

#ifdef ENABLE_DISCORD_RPC
static void HandleDiscordDisconnected(int error_code, const char* message) {
    LOG_ERROR(Frontend, "Discord RPC disconnected ({} {})", error_code, message);
}

static void HandleDiscordError(int error_code, const char* message) {
    LOG_ERROR(Frontend, "Discord RPC error ({} {})", error_code, message);
}
#endif

const int GMainWindow::max_recent_files_item;

GMainWindow::GMainWindow() : config{new Config()}, emu_thread{nullptr} {
    Log::Filter log_filter;
    log_filter.ParseFilterString(Settings::values.log_filter);
    Log::SetGlobalFilter(log_filter);
    Log::AddBackend(
        std::make_unique<Log::FileBackend>(FileUtil::GetUserPath(D_USER_IDX) + LOG_FILE));
    Util::ToggleConsole();
    Settings::LogSettings();

    // register types to use in slots and signals
    qRegisterMetaType<std::size_t>("std::size_t");
    qRegisterMetaType<Service::AM::InstallStatus>("Service::AM::InstallStatus");

    setAcceptDrops(true);
    ui.setupUi(this);
    statusBar()->hide();

    default_theme_paths = QIcon::themeSearchPaths();
    UpdateUITheme();

    Network::Init();

    InitializeWidgets();
    InitializeRecentFileMenuActions();
    InitializeHotkeys();

    SetDefaultUIGeometry();
    RestoreUIState();

    ConnectMenuEvents();
    ConnectWidgetEvents();

    SetupUIStrings();

#ifdef _MSC_VER
    int cpu_id[4];
    __cpuid(cpu_id, 1);
    if (!((cpu_id[2] >> 19) & 1)) {
        QMessageBox::critical(this, "Citra", "Your CPU does not support SSE4.1");
        closeEvent(nullptr);
    }
#endif

    game_list->PopulateAsync(UISettings::values.game_dirs);

    QStringList args{QApplication::arguments()};
    if (args.length() >= 2) {
        BootGame(args[1]);
    }
}

GMainWindow::~GMainWindow() {
    // will get automatically deleted otherwise
    if (screens->parent() == nullptr)
        delete screens;
    Network::Shutdown();
}

void GMainWindow::InitializeWidgets() {
    screens = new Screens(this, emu_thread.get());
    screens->hide();

    game_list = new GameList(this);
    ui.horizontalLayout->addWidget(game_list);

    game_list_placeholder = new GameListPlaceholder(this);
    ui.horizontalLayout->addWidget(game_list_placeholder);
    game_list_placeholder->setVisible(false);

    multiplayer_state = new MultiplayerState(this, game_list->GetModel(), ui.action_Leave_Room,
                                             ui.action_Show_Room);
    multiplayer_state->setVisible(false);

    // Create status bar
    message_label = new QLabel();

    // Configured separately for left alignment
    message_label->setVisible(false);
    message_label->setFrameStyle(QFrame::NoFrame);
    message_label->setContentsMargins(4, 0, 4, 0);
    message_label->setAlignment(Qt::AlignLeft);
    statusBar()->addPermanentWidget(message_label, 1);

    progress_bar = new QProgressBar();
    progress_bar->setMaximum(INT_MAX);
    progress_bar->hide();
    statusBar()->addPermanentWidget(progress_bar);

    emu_speed_label = new QLabel();
    emu_speed_label->setToolTip("Current emulation speed. Values higher or lower than 100% "
                                "indicate emulation is running faster or slower than a 3DS.");
    game_fps_label = new QLabel();
    game_fps_label->setToolTip("How many frames per second the game is currently displaying. "
                               "This will vary from game to game and scene to scene.");
    emu_frametime_label = new QLabel();
    emu_frametime_label->setToolTip(
        "Time taken to emulate a 3DS frame, not counting framelimiting. For "
        "full-speed emulation (with screen refresh rate set to 60) this should be at most 16.67 "
        "ms.");

    for (auto& label : {emu_speed_label, game_fps_label, emu_frametime_label}) {
        label->setVisible(false);
        label->setFrameStyle(QFrame::NoFrame);
        label->setContentsMargins(4, 0, 4, 0);
        statusBar()->addPermanentWidget(label, 0);
    }
    statusBar()->addPermanentWidget(multiplayer_state->GetStatusIcon(), 0);
    statusBar()->setVisible(true);

    // Removes an ugly inner border from the status bar widgets under Linux
    setStyleSheet("QStatusBar::item{border: none;}");

    QActionGroup* actionGroup_ScreenLayouts{new QActionGroup(this)};
    actionGroup_ScreenLayouts->addAction(ui.action_Screen_Layout_Default);
    actionGroup_ScreenLayouts->addAction(ui.action_Screen_Layout_Single_Screen);
    actionGroup_ScreenLayouts->addAction(ui.action_Screen_Layout_Medium_Screen);
    actionGroup_ScreenLayouts->addAction(ui.action_Screen_Layout_Large_Screen);
    actionGroup_ScreenLayouts->addAction(ui.action_Screen_Layout_Side_by_Side);
}

void GMainWindow::InitializeRecentFileMenuActions() {
    for (int i{}; i < max_recent_files_item; ++i) {
        actions_recent_files[i] = new QAction(this);
        actions_recent_files[i]->setVisible(false);

        connect(actions_recent_files[i], &QAction::triggered, this, &GMainWindow::OnMenuRecentFile);

        ui.menu_recent_files->addAction(actions_recent_files[i]);
    }
    ui.menu_recent_files->addSeparator();
    QAction* action_clear_recent_files{new QAction(this)};
    action_clear_recent_files->setText("Clear Recent Files");
    connect(action_clear_recent_files, &QAction::triggered, this, [this] {
        UISettings::values.recent_files.clear();
        UpdateRecentFiles();
    });
    ui.menu_recent_files->addAction(action_clear_recent_files);

    UpdateRecentFiles();
}

void GMainWindow::InitializeHotkeys() {
    RegisterHotkey("Main Window", "Load File", QKeySequence::Open);
    RegisterHotkey("Main Window", "Start Emulation");
    RegisterHotkey("Main Window", "Continue/Pause", QKeySequence(Qt::Key_F4));
    RegisterHotkey("Main Window", "Restart", QKeySequence(Qt::Key_F5));
    RegisterHotkey("Main Window", "Swap Screens", QKeySequence("F9"));
    RegisterHotkey("Main Window", "Toggle Screen Layout", QKeySequence("F10"));
    RegisterHotkey("Main Window", "Fullscreen", QKeySequence::FullScreen);
    RegisterHotkey("Main Window", "Exit Fullscreen", QKeySequence(Qt::Key_Escape),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Toggle Speed Limit", QKeySequence("CTRL+Z"),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Increase Speed Limit", QKeySequence("+"),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Decrease Speed Limit", QKeySequence("-"),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Increase Internal Resolution", QKeySequence("CTRL+I"),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Decrease Internal Resolution", QKeySequence("CTRL+D"),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Capture Screenshot", QKeySequence("CTRL+S"));
    RegisterHotkey("Main Window", "Toggle Sleep Mode", QKeySequence("F2"));
    RegisterHotkey("Main Window", "Change CPU Ticks", QKeySequence("CTRL+T"));
    RegisterHotkey("Main Window", "Toggle Frame Advancing", QKeySequence("CTRL+A"),
                   Qt::ApplicationShortcut);
    RegisterHotkey("Main Window", "Advance Frame", QKeySequence(Qt::Key_Backslash),
                   Qt::ApplicationShortcut);
    LoadHotkeys();

    connect(GetHotkey("Main Window", "Load File", this), &QShortcut::activated, this,
            &GMainWindow::OnMenuLoadFile);
    connect(GetHotkey("Main Window", "Start Emulation", this), &QShortcut::activated, this,
            &GMainWindow::OnStartGame);
    connect(GetHotkey("Main Window", "Continue/Pause", this), &QShortcut::activated, this, [&] {
        if (emulation_running) {
            if (Core::System::GetInstance().IsRunning()) {
                OnPauseGame();
            } else {
                OnStartGame();
            }
        }
    });
    connect(GetHotkey("Main Window", "Restart", this), &QShortcut::activated, this, [this] {
        if (!Core::System::GetInstance().IsPoweredOn())
            return;
        BootGame(QString(game_path));
    });
    connect(GetHotkey("Main Window", "Swap Screens", screens), &QShortcut::activated,
            ui.action_Screen_Layout_Swap_Screens, &QAction::trigger);
    connect(GetHotkey("Main Window", "Toggle Screen Layout", screens), &QShortcut::activated, this,
            &GMainWindow::ToggleScreenLayout);
    connect(GetHotkey("Main Window", "Fullscreen", screens), &QShortcut::activated,
            ui.action_Fullscreen, &QAction::trigger);
    connect(GetHotkey("Main Window", "Fullscreen", screens), &QShortcut::activatedAmbiguously,
            ui.action_Fullscreen, &QAction::trigger);
    connect(GetHotkey("Main Window", "Exit Fullscreen", this), &QShortcut::activated, this, [&] {
        if (emulation_running) {
            ui.action_Fullscreen->setChecked(false);
            ToggleFullscreen();
        }
    });
    connect(GetHotkey("Main Window", "Toggle Speed Limit", this), &QShortcut::activated, this, [&] {
        Settings::values.use_frame_limit = !Settings::values.use_frame_limit;
        UpdateStatusBar();
    });
    constexpr u16 SPEED_LIMIT_STEP{5};
    connect(GetHotkey("Main Window", "Increase Speed Limit", this), &QShortcut::activated, this,
            [&] {
                if (Settings::values.frame_limit < 9999 - SPEED_LIMIT_STEP) {
                    Settings::values.frame_limit += SPEED_LIMIT_STEP;
                    UpdateStatusBar();
                }
            });
    connect(GetHotkey("Main Window", "Decrease Speed Limit", this), &QShortcut::activated, this,
            [&] {
                if (Settings::values.frame_limit > SPEED_LIMIT_STEP) {
                    Settings::values.frame_limit -= SPEED_LIMIT_STEP;
                    UpdateStatusBar();
                }
            });
    connect(GetHotkey("Main Window", "Increase Internal Resolution", this), &QShortcut::activated,
            this, [&] {
                if (Settings::values.resolution_factor < 10) {
                    ++Settings::values.resolution_factor;
                }
            });
    connect(GetHotkey("Main Window", "Decrease Internal Resolution", this), &QShortcut::activated,
            this, [&] {
                if (Settings::values.resolution_factor > 0) {
                    --Settings::values.resolution_factor;
                }
            });
    connect(GetHotkey("Main Window", "Capture Screenshot", this), &QShortcut::activated, this, [&] {
        if (Core::System::GetInstance().IsRunning()) {
            OnCaptureScreenshot();
        }
    });
    connect(GetHotkey("Main Window", "Toggle Sleep Mode", this), &QShortcut::activated, this, [&] {
        auto& system{Core::System::GetInstance()};
        if (system.IsPoweredOn()) {
            bool new_value{!system.IsSleepModeEnabled()};
            system.SetSleepModeEnabled(new_value);
            ui.action_Sleep_Mode->setChecked(new_value);
        }
    });
    connect(GetHotkey("Main Window", "Change CPU Ticks", this), &QShortcut::activated, this, [&] {
        QString str{QInputDialog::getText(this, "Change CPU Ticks", "Ticks:")};
        if (str.isEmpty())
            return;
        bool ok{};
        u64 ticks{str.toULongLong(&ok)};
        if (ok) {
            Settings::values.ticks_mode = Settings::TicksMode::Custom;
            Settings::values.ticks = ticks;

            if (Core::System::GetInstance().IsPoweredOn()) {
                Core::GetCPU().SyncSettings();
            }
        } else {
            QMessageBox::critical(this, "Error", "Invalid number");
        }
    });
    connect(GetHotkey("Main Window", "Toggle Frame Advancing", this), &QShortcut::activated,
            ui.action_Enable_Frame_Advancing, &QAction::trigger);
    connect(GetHotkey("Main Window", "Advance Frame", this), &QShortcut::activated,
            ui.action_Advance_Frame, &QAction::trigger);
}

void GMainWindow::SetDefaultUIGeometry() {
    // geometry: 55% of the window contents are in the upper screen half, 45% in the lower half
    const QRect screenRect{QApplication::desktop()->screenGeometry(this)};

    const int w{screenRect.width() * 2 / 3};
    const int h{screenRect.height() / 2};
    const int x{(screenRect.x() + screenRect.width()) / 2 - w / 2};
    const int y{(screenRect.y() + screenRect.height()) / 2 - h * 55 / 100};

    setGeometry(x, y, w, h);
}

void GMainWindow::RestoreUIState() {
    restoreGeometry(UISettings::values.geometry);
    restoreState(UISettings::values.state);
    screens->restoreGeometry(UISettings::values.screens_geometry);

    game_list->LoadInterfaceLayout();

    screens->BackupGeometry();
    ui.horizontalLayout->addWidget(screens);
    screens->setFocusPolicy(Qt::ClickFocus);
    if (emulation_running) {
        screens->setVisible(true);
        screens->setFocus();
        game_list->hide();
    }

    ui.action_Fullscreen->setChecked(UISettings::values.fullscreen);
    SyncMenuUISettings();

    ui.action_Show_Filter_Bar->setChecked(UISettings::values.show_filter_bar);
    game_list->setFilterVisible(ui.action_Show_Filter_Bar->isChecked());

    ui.action_Show_Status_Bar->setChecked(UISettings::values.show_status_bar);
    statusBar()->setVisible(ui.action_Show_Status_Bar->isChecked());
}

void GMainWindow::ConnectWidgetEvents() {
    connect(game_list, &GameList::GameChosen, this, &GMainWindow::OnGameListLoadFile);
    connect(game_list, &GameList::OpenDirectory, this, &GMainWindow::OnGameListOpenDirectory);
    connect(game_list, &GameList::OpenFolderRequested, this, &GMainWindow::OnGameListOpenFolder);
    connect(game_list, &GameList::AddDirectory, this, &GMainWindow::OnGameListAddDirectory);
    connect(game_list_placeholder, &GameListPlaceholder::AddDirectory, this,
            &GMainWindow::OnGameListAddDirectory);
    connect(game_list, &GameList::ShowList, this, &GMainWindow::OnGameListShowList);

    connect(this, &GMainWindow::EmulationStarting, screens, &Screens::OnEmulationStarting);
    connect(this, &GMainWindow::EmulationStopping, screens, &Screens::OnEmulationStopping);

    connect(&status_bar_update_timer, &QTimer::timeout, this, &GMainWindow::UpdateStatusBar);

    connect(this, &GMainWindow::UpdateProgress, this, &GMainWindow::OnUpdateProgress);
    connect(this, &GMainWindow::CIAInstallReport, this, &GMainWindow::OnCIAInstallReport);
    connect(this, &GMainWindow::CIAInstallFinished, this, &GMainWindow::OnCIAInstallFinished);
    connect(this, &GMainWindow::UpdateThemedIcons, multiplayer_state,
            &MultiplayerState::UpdateThemedIcons);
}

void GMainWindow::ConnectMenuEvents() {
    // File
    connect(ui.action_Load_File, &QAction::triggered, this, &GMainWindow::OnMenuLoadFile);
    connect(ui.action_Install_CIA, &QAction::triggered, this, &GMainWindow::OnMenuInstallCIA);
    connect(ui.action_Add_Seed, &QAction::triggered, this, &GMainWindow::OnMenuAddSeed);
    connect(ui.action_Exit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui.action_Open_User_Directory, &QAction::triggered, this,
            &GMainWindow::OnOpenUserDirectory);
    connect(ui.action_Select_SDMC_Directory, &QAction::triggered, this,
            &GMainWindow::OnSelectSDMCDirectory);
    connect(ui.action_Load_Amiibo, &QAction::triggered, this, &GMainWindow::OnLoadAmiibo);
    connect(ui.action_Remove_Amiibo, &QAction::triggered, this, &GMainWindow::OnRemoveAmiibo);

    // Emulation
    connect(ui.action_Start, &QAction::triggered, this, &GMainWindow::OnStartGame);
    connect(ui.action_Pause, &QAction::triggered, this, &GMainWindow::OnPauseGame);
    connect(ui.action_Stop, &QAction::triggered, this, &GMainWindow::OnStopGame);
    connect(ui.action_Restart, &QAction::triggered, this, [this] { BootGame(QString(game_path)); });
    connect(ui.action_Sleep_Mode, &QAction::triggered, this, &GMainWindow::ToggleSleepMode);
    connect(ui.action_Configuration, &QAction::triggered, this, &GMainWindow::OnOpenConfiguration);
    connect(ui.action_Cheats, &QAction::triggered, this, &GMainWindow::OnCheats);
    connect(ui.action_Control_Panel, &QAction::triggered, this, &GMainWindow::OnControlPanel);

    // View
    ui.action_Show_Filter_Bar->setShortcut(QKeySequence("CTRL+F"));
    connect(ui.action_Show_Filter_Bar, &QAction::triggered, this, &GMainWindow::OnToggleFilterBar);
    connect(ui.action_Show_Status_Bar, &QAction::triggered, statusBar(), &QStatusBar::setVisible);
    ui.action_Fullscreen->setShortcut(GetHotkey("Main Window", "Fullscreen", this)->key());
    ui.action_Screen_Layout_Swap_Screens->setShortcut(
        GetHotkey("Main Window", "Swap Screens", this)->key());
    ui.action_Screen_Layout_Swap_Screens->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(ui.action_Fullscreen, &QAction::triggered, this, &GMainWindow::ToggleFullscreen);

    connect(ui.action_Screen_Layout_Default, &QAction::triggered, this,
            &GMainWindow::ChangeScreenLayout);
    connect(ui.action_Screen_Layout_Single_Screen, &QAction::triggered, this,
            &GMainWindow::ChangeScreenLayout);
    connect(ui.action_Screen_Layout_Medium_Screen, &QAction::triggered, this,
            &GMainWindow::ChangeScreenLayout);
    connect(ui.action_Screen_Layout_Large_Screen, &QAction::triggered, this,
            &GMainWindow::ChangeScreenLayout);
    connect(ui.action_Screen_Layout_Side_by_Side, &QAction::triggered, this,
            &GMainWindow::ChangeScreenLayout);
    connect(ui.action_Screen_Layout_Swap_Screens, &QAction::triggered, this,
            &GMainWindow::OnSwapScreens);

    // Tools
    connect(ui.action_Record_Movie, &QAction::triggered, this, &GMainWindow::OnRecordMovie);
    connect(ui.action_Play_Movie, &QAction::triggered, this, &GMainWindow::OnPlayMovie);
    connect(ui.action_Stop_Recording_Playback, &QAction::triggered, this,
            &GMainWindow::OnStopRecordingPlayback);
    connect(ui.action_Capture_Screenshot, &QAction::triggered, this,
            &GMainWindow::OnCaptureScreenshot);
    connect(ui.action_Set_Play_Coins, &QAction::triggered, this, &GMainWindow::OnSetPlayCoins);
    connect(ui.action_Enable_Frame_Advancing, &QAction::triggered, this, [this] {
        if (emulation_running) {
            Core::System::GetInstance().frame_limiter.SetFrameAdvancing(
                ui.action_Enable_Frame_Advancing->isChecked());
            ui.action_Advance_Frame->setEnabled(ui.action_Enable_Frame_Advancing->isChecked());
        }
    });
    connect(ui.action_Advance_Frame, &QAction::triggered, this, [this] {
        if (emulation_running) {
            ui.action_Enable_Frame_Advancing->setChecked(true);
            ui.action_Advance_Frame->setEnabled(true);
            Core::System::GetInstance().frame_limiter.AdvanceFrame();
        }
    });

    // Multiplayer
    connect(ui.action_View_Lobby, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnViewLobby);
    connect(ui.action_Start_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnCreateRoom);
    connect(ui.action_Leave_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnCloseRoom);
    connect(ui.action_Connect_To_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnDirectConnectToRoom);
    connect(ui.action_Show_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnOpenNetworkRoom);

    // Help
    connect(ui.action_About, &QAction::triggered, this, &GMainWindow::OnMenuAboutCitra);
}

bool GMainWindow::LoadROM(const QString& filename) {
    // Shutdown previous session if the emu thread is still active...
    if (emu_thread != nullptr)
        ShutdownGame();

    screens->InitRenderTarget();
    screens->MakeCurrent();

    if (!gladLoadGL()) {
        QMessageBox::critical(this, "OpenGL 3.3 Unsupported",
                              "Your GPU may not support OpenGL 3.3, or you do not "
                              "have the latest graphics driver.");
        return false;
    }

    auto& system{Core::System::GetInstance()};

    const Core::System::ResultStatus result{system.Load(filename.toStdString())};

    if (result != Core::System::ResultStatus::Success) {
        switch (result) {
        case Core::System::ResultStatus::ErrorGetLoader:
            LOG_CRITICAL(Frontend, "Failed to obtain loader for {}!", filename.toStdString());
            QMessageBox::critical(
                this, "Invalid ROM Format",
                "Your ROM format is not supported.<br/>Please follow the guides to redump your "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Game-Cartridges/'>game "
                "cartridges</a> or "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Installed-Titles/'>installed "
                "titles</a>.");
            break;

        case Core::System::ResultStatus::ErrorSystemMode:
            LOG_CRITICAL(Frontend, "Failed to load ROM!");
            QMessageBox::critical(
                this, "ROM Corrupted",
                "Your ROM is corrupted. <br/>Please follow the guides to redump your "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Game-Cartridges/'>game "
                "cartridges</a> or "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Installed-Titles/'>installed "
                "titles</a>.");
            break;

        case Core::System::ResultStatus::ErrorLoader_ErrorEncrypted: {
            QMessageBox::critical(
                this, "ROM Encrypted",
                "Your ROM is encrypted. <br/>Please follow the guides to redump your "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Game-Cartridges/'>game "
                "cartridges</a> or "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Installed-Titles/'>installed "
                "titles</a>.");
            break;
        }
        case Core::System::ResultStatus::ErrorLoader_ErrorInvalidFormat:
            QMessageBox::critical(
                this, "Invalid ROM Format",
                "Your ROM format is not supported.<br/>Please follow the guides to redump your "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Game-Cartridges/'>game "
                "cartridges</a> or "
                "<a "
                "href='https://github.com/citra-valentin/citra/wiki/"
                "Dumping-Installed-Titles/'>installed "
                "titles</a>.");
            break;

        case Core::System::ResultStatus::ErrorVideoCore:
            QMessageBox::critical(
                this, "Video Core Error",
                "An error has occured. Please see the log for more details.<br/>Ensure that you "
                "have the latest graphics drivers for your GPU.");
            break;

        case Core::System::ResultStatus::ErrorVideoCore_ErrorGenericDrivers:
            QMessageBox::critical(
                this, "Video Core Error",
                "You are running default Windows drivers "
                "for your GPU. You need to install the "
                "proper drivers for your graphics card from the manufacturer's website.");
            break;

        case Core::System::ResultStatus::ErrorVideoCore_ErrorBelowGL33:
            QMessageBox::critical(this, "OpenGL 3.3 Unsupported",
                                  "Your GPU may not support OpenGL 3.3, or you do not "
                                  "have the latest graphics driver.");
            break;

        default:
            QMessageBox::critical(this, "Error while loading ROM!",
                                  "An unknown error occured. Please see the log for more details.");
            break;
        }
        return false;
    }

    std::string title;
    system.GetAppLoader().ReadTitle(title);
    game_title = QString::fromStdString(title);
    SetupUIStrings();

    game_path = filename;

#ifdef ENABLE_DISCORD_RPC
    DiscordEventHandlers handlers{};
    handlers.disconnected = HandleDiscordDisconnected;
    handlers.errored = HandleDiscordError;
    Discord_Initialize("472104565165260826", &handlers, 0, NULL);
    DiscordRichPresence presence{};
    presence.state = title.empty() ? "Unknown game" : title.c_str();
    presence.details = "Playing";
    presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();
    presence.largeImageKey = "icon";
    Discord_UpdatePresence(&presence);
#endif

    if (cheats_window != nullptr)
        cheats_window->UpdateTitleID();
    else
        CheatCore::RefreshCheats();

    return true;
}

void GMainWindow::BootGame(const QString& filename) {
    LOG_INFO(Frontend, "Booting {}", filename.toStdString());
    StoreRecentFile(filename); // Put the filename on top of the list

    if (movie_record_on_start) {
        Core::Movie::GetInstance().PrepareForRecording();
    }

    if (!LoadROM(filename))
        return;

    // Create and start the emulation thread
    emu_thread = std::make_unique<EmuThread>(screens);
    emit EmulationStarting(emu_thread.get());
    screens->moveContext();
    emu_thread->start();

    connect(screens, &Screens::Closed, this, &GMainWindow::OnStopGame);

    // Update the GUI
    game_list->hide();
    game_list_placeholder->hide();
    ui.action_Sleep_Mode->setEnabled(true);
    ui.action_Sleep_Mode->setChecked(false);

    status_bar_update_timer.start(2000);

    screens->show();
    screens->setFocus();

    emulation_running = true;

    if (ui.action_Fullscreen->isChecked()) {
        ShowFullscreen();
    }

    OnStartGame();

    HLE::Applets::ErrEula::cb = [this](HLE::Applets::ErrEulaConfig& config) {
        applet_open = true;
        ErrEulaCallback(config);
        std::unique_lock<std::mutex> lock{applet_mutex};
        applet_cv.wait(lock, [&] { return !applet_open; });
    };

    HLE::Applets::SoftwareKeyboard::cb = [this](HLE::Applets::SoftwareKeyboardConfig& config,
                                                std::u16string& text) {
        applet_open = true;
        SwkbdCallback(config, text);
        std::unique_lock<std::mutex> lock{applet_mutex};
        applet_cv.wait(lock, [&] { return !applet_open; });
    };

    HLE::Applets::MiiSelector::cb = [this](const HLE::Applets::MiiConfig& config,
                                           HLE::Applets::MiiResult& result) {
        applet_open = true;
        MiiSelectorCallback(config, result);
        std::unique_lock<std::mutex> lock{applet_mutex};
        applet_cv.wait(lock, [&] { return !applet_open; });
    };

    SharedPage::Handler::update_3d = [this] { Update3D(); };

    RPC::RPCServer::cb_update_frame_advancing = [this] { UpdateFrameAdvancingCallback(); };

    Service::NWM::NWM_EXT::update_control_panel = [this] { UpdateControlPanelNetwork(); };
}

void GMainWindow::ShutdownGame() {
    OnStopRecordingPlayback();
    emu_thread->RequestStop();

    // Frame advancing must be cancelled in order to release the emu thread from waiting
    Core::System::GetInstance().frame_limiter.SetFrameAdvancing(false);

    emit EmulationStopping();

    // Wait for emulation thread to complete and delete it
    emu_thread->wait();
    emu_thread = nullptr;

    Camera::QtMultimediaCameraHandler::ReleaseHandlers();

    // The emulation is stopped, so closing the window or not does not matter anymore
    disconnect(screens, &Screens::Closed, this, &GMainWindow::OnStopGame);

    // Update the GUI
    ui.action_Start->setEnabled(false);
    ui.action_Start->setText("Start");
    ui.action_Pause->setEnabled(false);
    ui.action_Stop->setEnabled(false);
    ui.action_Restart->setEnabled(false);
    ui.action_Cheats->setEnabled(false);
    ui.action_Select_SDMC_Directory->setEnabled(true);
    ui.action_Capture_Screenshot->setEnabled(false);
    ui.action_Load_Amiibo->setEnabled(false);
    ui.action_Remove_Amiibo->setEnabled(false);
    ui.action_Enable_Frame_Advancing->setEnabled(false);
    ui.action_Enable_Frame_Advancing->setChecked(false);
    ui.action_Advance_Frame->setEnabled(false);
    ui.action_Sleep_Mode->setEnabled(false);
    ui.action_Sleep_Mode->setChecked(false);
    screens->hide();
    if (game_list->isEmpty())
        game_list_placeholder->show();
    else
        game_list->show();
    game_list->setFilterFocus();

    // Disable status bar updates
    status_bar_update_timer.stop();
    message_label->setVisible(false);
    emu_speed_label->setVisible(false);
    game_fps_label->setVisible(false);
    emu_frametime_label->setVisible(false);

    emulation_running = false;

    game_title.clear();
    SetupUIStrings();

    game_path.clear();

#ifdef ENABLE_DISCORD_RPC
    Discord_ClearPresence();
    Discord_Shutdown();
#endif
}

void GMainWindow::StoreRecentFile(const QString& filename) {
    UISettings::values.recent_files.prepend(filename);
    UISettings::values.recent_files.removeDuplicates();
    while (UISettings::values.recent_files.size() > max_recent_files_item) {
        UISettings::values.recent_files.removeLast();
    }

    UpdateRecentFiles();
}

void GMainWindow::ErrEulaCallback(HLE::Applets::ErrEulaConfig& config) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "ErrEulaCallback", Qt::BlockingQueuedConnection,
                                  Q_ARG(HLE::Applets::ErrEulaConfig&, config));
        return;
    }

    std::unique_lock<std::mutex> lock{applet_mutex};

    switch (config.error_type) {
    case HLE::Applets::ErrEulaErrorType::ErrorCode: {
        QMessageBox::critical(
            nullptr, "ErrEula",
            QString("Error Code: %1")
                .arg(QString::fromStdString(fmt::format("0x{:08X}", config.error_code))));
        break;
    }
    case HLE::Applets::ErrEulaErrorType::LocalizedErrorText:
    case HLE::Applets::ErrEulaErrorType::ErrorText: {
        std::string error{Common::UTF16ToUTF8(config.error_text)};
        QMessageBox::critical(
            nullptr, "ErrEula",
            QString("Error Code: %1\n\n%2")
                .arg(QString::fromStdString(fmt::format("0x{:08X}", config.error_code)),
                     QString::fromStdString(error)));
        break;
    }
    case HLE::Applets::ErrEulaErrorType::Agree:
    case HLE::Applets::ErrEulaErrorType::Eula:
    case HLE::Applets::ErrEulaErrorType::EulaDrawOnly:
    case HLE::Applets::ErrEulaErrorType::EulaFirstBoot: {
        QMessageBox::StandardButton button{
            QMessageBox::question(nullptr, "ErrEula", "Agree EULA?")};
        if (button == QMessageBox::StandardButton::Yes) {
            Service::CFG::GetCurrentModule()->AgreeEula();
        }
        break;
    }
    }

    config.return_code = HLE::Applets::ErrEulaResult::Success;
    applet_open = false;
}

void GMainWindow::SwkbdCallback(HLE::Applets::SoftwareKeyboardConfig& config,
                                std::u16string& text) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "SwkbdCallback", Qt::BlockingQueuedConnection,
                                  Q_ARG(HLE::Applets::SoftwareKeyboardConfig&, config),
                                  Q_ARG(std::u16string&, text));
        return;
    }

    std::unique_lock<std::mutex> lock{applet_mutex};
    SoftwareKeyboardDialog dialog{this, config, text};
    dialog.exec();

    applet_open = false;
}

void GMainWindow::MiiSelectorCallback(const HLE::Applets::MiiConfig& config,
                                      HLE::Applets::MiiResult& result) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "MiiSelectorCallback", Qt::BlockingQueuedConnection,
                                  Q_ARG(const HLE::Applets::MiiConfig&, config),
                                  Q_ARG(HLE::Applets::MiiResult&, result));
        return;
    }

    std::unique_lock<std::mutex> lock{applet_mutex};
    MiiSelectorDialog dialog{this, config, result};
    dialog.exec();

    applet_open = false;
}

void GMainWindow::Update3D() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "Update3D", Qt::BlockingQueuedConnection);
        return;
    }

    if (control_panel != nullptr) {
        control_panel->Update3D();
    }
}

void GMainWindow::UpdateFrameAdvancingCallback() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "UpdateFrameAdvancingCallback",
                                  Qt::BlockingQueuedConnection);
        return;
    }

    const bool enabled{Core::System::GetInstance().frame_limiter.GetFrameAdvancing()};
    ui.action_Enable_Frame_Advancing->setChecked(enabled);
    ui.action_Advance_Frame->setEnabled(enabled);
}

void GMainWindow::UpdateControlPanelNetwork() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "UpdateControlPanelNetwork", Qt::BlockingQueuedConnection);
        return;
    }

    if (control_panel != nullptr) {
        control_panel->UpdateNetwork();
    }
}

void GMainWindow::UpdateRecentFiles() {
    const int num_recent_files{
        std::min(UISettings::values.recent_files.size(), max_recent_files_item)};

    for (int i{}; i < num_recent_files; i++) {
        const QString text{QString("%1. %2").arg(i + 1).arg(
            QFileInfo(UISettings::values.recent_files[i]).fileName())};
        actions_recent_files[i]->setText(text);
        actions_recent_files[i]->setData(UISettings::values.recent_files[i]);
        actions_recent_files[i]->setToolTip(UISettings::values.recent_files[i]);
        actions_recent_files[i]->setVisible(true);
    }

    for (int j{num_recent_files}; j < max_recent_files_item; ++j) {
        actions_recent_files[j]->setVisible(false);
    }

    // Enable the recent files menu if the list isn't empty
    ui.menu_recent_files->setEnabled(num_recent_files != 0);
}

void GMainWindow::OnGameListLoadFile(QString game_path) {
    BootGame(game_path);
}

void GMainWindow::OnGameListOpenFolder(u64 data_id, GameListOpenTarget target) {
    std::string path;
    std::string open_target;

    switch (target) {
    case GameListOpenTarget::SAVE_DATA: {
        open_target = "Save Data";
        std::string sdmc_dir{FileUtil::GetUserPath(D_SDMC_IDX, Settings::values.sdmc_dir + "/")};
        path = FileSys::ArchiveSource_SDSaveData::GetSaveDataPathFor(sdmc_dir, data_id);
        break;
    }
    case GameListOpenTarget::EXT_DATA: {
        open_target = "Extra Data";
        std::string sdmc_dir{FileUtil::GetUserPath(D_SDMC_IDX, Settings::values.sdmc_dir + "/")};
        path = FileSys::GetExtDataPathFromId(sdmc_dir, data_id);
        break;
    }
    case GameListOpenTarget::APPLICATION: {
        open_target = "Application";
        path = Service::AM::GetTitlePath(Service::AM::GetTitleMediaType(data_id), data_id) +
               "content/";
        break;
    }
    case GameListOpenTarget::UPDATE_DATA:
        open_target = "Update Data";
        path = Service::AM::GetTitlePath(Service::FS::MediaType::SDMC, data_id + 0xe00000000) +
               "content/";
        break;
    default:
        LOG_ERROR(Frontend, "Unexpected target {}", static_cast<int>(target));
        return;
    }

    QString qpath{QString::fromStdString(path)};

    QDir dir{qpath};
    if (!dir.exists()) {
        QMessageBox::critical(
            this, QString("Error Opening %1 Folder").arg(QString::fromStdString(open_target)),
            "Folder does not exist!");
        return;
    }

    LOG_INFO(Frontend, "Opening {} path for data_id={:016x}", open_target, data_id);

    QDesktopServices::openUrl(QUrl::fromLocalFile(qpath));
}

void GMainWindow::OnGameListOpenDirectory(QString directory) {
    QString path{};
    if (directory == "INSTALLED") {
        path = QString::fromStdString(
            FileUtil::GetUserPath(D_SDMC_IDX, Settings::values.sdmc_dir + "/") +
            "Nintendo "
            "3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/00040000");
    } else if (directory == "SYSTEM") {
        path =
            QString::fromStdString(FileUtil::GetUserPath(D_NAND_IDX).c_str() +
                                   std::string("00000000000000000000000000000000/title/00040010"));
    } else {
        path = directory;
    }
    if (!QFileInfo::exists(path)) {
        QMessageBox::critical(this, QString("Error Opening %1").arg(path),
                              "Folder does not exist!");
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void GMainWindow::OnGameListAddDirectory() {
    QString dir_path{QFileDialog::getExistingDirectory(this, "Select Directory")};
    if (dir_path.isEmpty())
        return;
    UISettings::GameDir game_dir{dir_path, false, true};
    if (!UISettings::values.game_dirs.contains(game_dir)) {
        UISettings::values.game_dirs.append(game_dir);
        game_list->PopulateAsync(UISettings::values.game_dirs);
    } else {
        LOG_WARNING(Frontend, "Selected directory is already in the game list");
    }
}

void GMainWindow::OnGameListShowList(bool show) {
    if (emulation_running) {
        return;
    }
    game_list->setVisible(show);
    game_list_placeholder->setVisible(!show);
}

void GMainWindow::OnMenuLoadFile() {
    QString extensions{};
    for (const auto& piece : game_list->supported_file_extensions)
        extensions += "*." + piece + " ";

    QString file_filter{QString("3DS Executable (%1);;All Files (*.*)").arg(extensions)};

    QString filename{QFileDialog::getOpenFileName(this, "Load File", ".", file_filter)};
    if (!filename.isEmpty()) {
        BootGame(filename);
    }
}

void GMainWindow::OnMenuInstallCIA() {
    QStringList filepaths{QFileDialog::getOpenFileNames(
        this, "Install CIA", ".", "CTR Importable Archive (*.cia);;All Files (*.*)")};
    if (filepaths.isEmpty())
        return;

    ui.action_Install_CIA->setEnabled(false);
    game_list->setDirectoryWatcherEnabled(false);
    progress_bar->show();

    QtConcurrent::run([&, filepaths] {
        Service::AM::InstallStatus status;
        const auto cia_progress{
            [&](std::size_t written, std::size_t total) { emit UpdateProgress(written, total); }};
        for (const auto current_path : filepaths) {
            status = Service::AM::InstallCIA(current_path.toStdString(), cia_progress);
            emit CIAInstallReport(status, current_path);
        }
        emit CIAInstallFinished();
    });
}

void GMainWindow::OnMenuAddSeed() {
    QString filepath{QFileDialog::getOpenFileName(this, "Add Seed", ".")};
    if (filepath.isEmpty())
        return;
    QString program_id_s{QInputDialog::getText(this, "Add Seed", "Enter the program ID")};
    if (program_id_s.isEmpty())
        return;
    bool ok{};
    u64 program_id{program_id_s.toULongLong(&ok, 16)};
    if (!ok) {
        QMessageBox::critical(this, "Error", "Invalid program ID");
        return;
    }
    FileSys::Seed seed{};
    seed.title_id = program_id;
    FileUtil::IOFile file{filepath.toStdString(), "rb"};
    file.ReadBytes(seed.data.data(), seed.data.size());
    FileSys::SeedDB db;
    db.Load();
    db.Add(seed);
    db.Save();
    game_list->PopulateAsync(UISettings::values.game_dirs);
}

void GMainWindow::OnUpdateProgress(std::size_t written, std::size_t total) {
    progress_bar->setValue(
        static_cast<int>(INT_MAX * (static_cast<double>(written) / static_cast<double>(total))));
}

void GMainWindow::OnCIAInstallReport(Service::AM::InstallStatus status, QString filepath) {
    QString filename{QFileInfo(filepath).fileName()};
    switch (status) {
    case Service::AM::InstallStatus::Success:
        statusBar()->showMessage(QString("%1 installed").arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorFailedToOpenFile:
        QMessageBox::critical(this, "Unable to open File",
                              QString("Could not open %1").arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorAborted:
        QMessageBox::critical(
            this, "Installation aborted",
            QString("The installation of %1 was aborted. Please see the log for more details")
                .arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorInvalid:
        QMessageBox::critical(this, "Invalid File", QString("%1 is not a valid CIA").arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorEncrypted:
        QMessageBox::critical(this, "Encrypted File",
                              QString("%1 must be decrypted "
                                      "before being used with Citra. A real 3DS is required.")
                                  .arg(filename));
        break;
    }
}

void GMainWindow::OnCIAInstallFinished() {
    progress_bar->hide();
    progress_bar->setValue(0);
    game_list->setDirectoryWatcherEnabled(true);
    ui.action_Install_CIA->setEnabled(true);
    game_list->PopulateAsync(UISettings::values.game_dirs);

    auto& system{Core::System::GetInstance()};

    if (system.IsPoweredOn()) {
        auto u{system.ServiceManager().GetService<Service::AM::AM_U>("am:u")};
        auto am{u->GetModule()};
        am->ScanForAllTitles();
    }
}

void GMainWindow::OnMenuRecentFile() {
    QAction* action{qobject_cast<QAction*>(sender())};
    assert(action);

    const QString filename{action->data().toString()};
    if (QFileInfo::exists(filename)) {
        BootGame(filename);
    } else {
        // Display an error message and remove the file from the list.
        QMessageBox::information(this, "File not found",
                                 QString("File \"%1\" not found").arg(filename));

        UISettings::values.recent_files.removeOne(filename);
        UpdateRecentFiles();
    }
}

void GMainWindow::OnStartGame() {
    Camera::QtMultimediaCameraHandler::ResumeCameras();

    if (movie_record_on_start) {
        Core::Movie::GetInstance().StartRecording(movie_record_path.toStdString());
        movie_record_on_start = false;
        movie_record_path.clear();
    }

    qRegisterMetaType<Core::System::ResultStatus>("Core::System::ResultStatus");
    qRegisterMetaType<std::string>("std::string");
    connect(emu_thread.get(), &EmuThread::ErrorThrown, this, &GMainWindow::OnCoreError);

    Core::System::GetInstance().SetRunning(true);

    ui.action_Start->setEnabled(false);
    ui.action_Start->setText("Continue");
    ui.action_Pause->setEnabled(true);
    ui.action_Stop->setEnabled(true);
    ui.action_Restart->setEnabled(true);
    ui.action_Cheats->setEnabled(true);
    ui.action_Select_SDMC_Directory->setEnabled(false);
    ui.action_Capture_Screenshot->setEnabled(true);
    ui.action_Load_Amiibo->setEnabled(true);
    ui.action_Enable_Frame_Advancing->setEnabled(true);
}

void GMainWindow::OnPauseGame() {
    Core::System::GetInstance().SetRunning(false);
    Camera::QtMultimediaCameraHandler::StopCameras();
    ui.action_Start->setEnabled(true);
    ui.action_Pause->setEnabled(false);
    ui.action_Stop->setEnabled(true);
}

void GMainWindow::OnStopGame() {
    ShutdownGame();
    if (cheats_window != nullptr)
        cheats_window->close();
}

void GMainWindow::ToggleFullscreen() {
    if (!emulation_running) {
        return;
    }
    if (ui.action_Fullscreen->isChecked()) {
        ShowFullscreen();
    } else {
        HideFullscreen();
    }
}

void GMainWindow::ShowFullscreen() {
    UISettings::values.geometry = saveGeometry();
    ui.menubar->hide();
    statusBar()->hide();
    showFullScreen();
}

void GMainWindow::HideFullscreen() {
    statusBar()->setVisible(ui.action_Show_Status_Bar->isChecked());
    ui.menubar->show();
    showNormal();
    restoreGeometry(UISettings::values.geometry);
}

void GMainWindow::ChangeScreenLayout() {
    Settings::LayoutOption new_layout{Settings::LayoutOption::Default};

    if (ui.action_Screen_Layout_Default->isChecked()) {
        new_layout = Settings::LayoutOption::Default;
    } else if (ui.action_Screen_Layout_Single_Screen->isChecked()) {
        new_layout = Settings::LayoutOption::SingleScreen;
    } else if (ui.action_Screen_Layout_Medium_Screen->isChecked()) {
        new_layout = Settings::LayoutOption::MediumScreen;
    } else if (ui.action_Screen_Layout_Large_Screen->isChecked()) {
        new_layout = Settings::LayoutOption::LargeScreen;
    } else if (ui.action_Screen_Layout_Side_by_Side->isChecked()) {
        new_layout = Settings::LayoutOption::SideScreen;
    }

    Settings::values.layout_option = new_layout;
    Settings::Apply();
}

void GMainWindow::ToggleScreenLayout() {
    Settings::LayoutOption new_layout{Settings::LayoutOption::Default};

    switch (Settings::values.layout_option) {
    case Settings::LayoutOption::Default:
        new_layout = Settings::LayoutOption::SingleScreen;
        break;
    case Settings::LayoutOption::SingleScreen:
        new_layout = Settings::LayoutOption::MediumScreen;
        break;
    case Settings::LayoutOption::MediumScreen:
        new_layout = Settings::LayoutOption::LargeScreen;
        break;
    case Settings::LayoutOption::LargeScreen:
        new_layout = Settings::LayoutOption::SideScreen;
        break;
    case Settings::LayoutOption::SideScreen:
        new_layout = Settings::LayoutOption::Default;
        break;
    }

    Settings::values.layout_option = new_layout;
    SyncMenuUISettings();
    Settings::Apply();
}

void GMainWindow::OnSwapScreens() {
    Settings::values.swap_screen = ui.action_Screen_Layout_Swap_Screens->isChecked();
    Settings::Apply();
}

void GMainWindow::ToggleSleepMode() {
    Core::System::GetInstance().SetSleepModeEnabled(ui.action_Sleep_Mode->isChecked());
}

void GMainWindow::OnOpenConfiguration() {
    ConfigurationDialog configuration_dialog{this};
    auto old_theme{UISettings::values.theme};
    int old_profile{Settings::values.profile};
    auto old_profiles{Settings::values.profiles};
    auto result{configuration_dialog.exec()};
    if (result == QDialog::Accepted) {
        configuration_dialog.applyConfiguration();
        if (UISettings::values.theme != old_theme) {
            UpdateUITheme();
            emit UpdateThemedIcons();
        }
        SyncMenuUISettings();
        game_list->RefreshGameDirectory();
        config->Save();
    } else {
        Settings::values.profiles = old_profiles;
        Settings::LoadProfile(old_profile);
    }
}

void GMainWindow::OnCheats() {
    if (cheats_window == nullptr)
        cheats_window = std::make_shared<CheatDialog>(this);
    cheats_window->show();
}

void GMainWindow::OnControlPanel() {
    if (control_panel == nullptr)
        control_panel = std::make_shared<ControlPanel>(this);
    control_panel->show();
}

void GMainWindow::OnSetPlayCoins() {
    bool ok{};
    u16 play_coins{static_cast<u16>(
        QInputDialog::getInt(this, "Set Play Coins", "Play Coins:", 0, 0, 300, 1, &ok,
                             Qt::WindowSystemMenuHint | Qt::WindowTitleHint))};
    if (ok) {
        Service::PTM::SetPlayCoins(play_coins);
    }
}

void GMainWindow::OnOpenUserDirectory() {
    QString path{QString::fromStdString(FileUtil::GetUserPath(D_USER_IDX))};
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(path.replace("./user", QDir::currentPath() + "/user")));
}

void GMainWindow::OnSelectSDMCDirectory() {
    QString dir{QFileDialog::getExistingDirectory(this, "Set SD Card Directory", ".")};
    if (dir.isEmpty())
        return;
    Settings::values.sdmc_dir = dir.toStdString();
    game_list->PopulateAsync(UISettings::values.game_dirs);
}

void GMainWindow::OnLoadAmiibo() {
    OnPauseGame();
    const QString extensions{"*.bin"};
    const QString file_filter{QString("Amiibo File") + " (" + extensions + ");;" +
                              "All Files (*.*)"};
    const QString filename{QFileDialog::getOpenFileName(this, "Load Amiibo", ".", file_filter)};
    OnStartGame();
    if (!filename.isEmpty()) {
        auto nfc{Core::System::GetInstance()
                     .ServiceManager()
                     .GetService<Service::NFC::Module::Interface>("nfc:u")};
        nfc->LoadAmiibo(filename.toStdString());
        ui.action_Remove_Amiibo->setEnabled(true);
    }
}

void GMainWindow::OnRemoveAmiibo() {
    auto nfc{
        Core::System::GetInstance().ServiceManager().GetService<Service::NFC::Module::Interface>(
            "nfc:u")};
    nfc->RemoveAmiibo();
    ui.action_Remove_Amiibo->setEnabled(false);
}

void GMainWindow::OnToggleFilterBar() {
    game_list->setFilterVisible(ui.action_Show_Filter_Bar->isChecked());
    if (ui.action_Show_Filter_Bar->isChecked()) {
        game_list->setFilterFocus();
    } else {
        game_list->clearFilter();
    }
}

void GMainWindow::OnRecordMovie() {
    if (emulation_running) {
        QMessageBox::StandardButton answer{QMessageBox::warning(
            this, "Record Movie",
            "To keep consistency with the RNG, it is recommended to record the movie from game "
            "start.<br>Are you sure you still want to record movies now?",
            QMessageBox::Yes | QMessageBox::No)};
        if (answer == QMessageBox::No)
            return;
    }
    const QString path{
        QFileDialog::getSaveFileName(this, "Record Movie", ".", "Citra TAS Movie (*.ctm)")};
    if (path.isEmpty())
        return;
    if (emulation_running) {
        Core::Movie::GetInstance().StartRecording(path.toStdString());
    } else {
        movie_record_on_start = true;
        movie_record_path = path;
        QMessageBox::information(this, "Record Movie",
                                 "Recording will start once you boot a game.");
    }
    ui.action_Record_Movie->setEnabled(false);
    ui.action_Play_Movie->setEnabled(false);
    ui.action_Stop_Recording_Playback->setEnabled(true);
}

bool GMainWindow::ValidateMovie(const QString& path, u64 program_id) {
    using namespace Core;
    Movie::ValidationResult result{
        Core::Movie::GetInstance().ValidateMovie(path.toStdString(), program_id)};
    const QString revision_dismatch_text{
        "The movie file you are trying to load was created on a different revision of Citra."
        "<br/>Citra has had some changes during the time, and the playback may desync or not "
        "work as expected."
        "<br/><br/>Are you sure you still want to load the movie file?"};
    const QString game_dismatch_text{
        "The movie file you are trying to load was recorded with a different game."
        "<br/>The playback may not work as expected, and it may cause unexpected results."
        "<br/><br/>Are you sure you still want to load the movie file?"};
    const QString invalid_movie_text{
        "The movie file you are trying to load is invalid."
        "<br/>Either the file is corrupted, or Citra has had made some major changes to the "
        "Movie module."
        "<br/>Please choose a different movie file and try again."};
    int answer{};
    switch (result) {
    case Movie::ValidationResult::RevisionDismatch:
        answer = QMessageBox::question(this, "Revision Dismatch", revision_dismatch_text,
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return false;
        break;
    case Movie::ValidationResult::GameDismatch:
        answer = QMessageBox::question(this, "Game Dismatch", game_dismatch_text,
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return false;
        break;
    case Movie::ValidationResult::Invalid:
        QMessageBox::critical(this, "Invalid Movie File", invalid_movie_text);
        return false;
    default:
        break;
    }
    return true;
}

void GMainWindow::OnPlayMovie() {
    if (emulation_running) {
        QMessageBox::StandardButton answer{QMessageBox::warning(
            this, "Play Movie",
            "To keep consistency with the RNG, it is recommended to play the movie from game "
            "start.<br>Are you sure you still want to play movies now?",
            QMessageBox::Yes | QMessageBox::No)};
        if (answer == QMessageBox::No)
            return;
    }

    const QString path{
        QFileDialog::getOpenFileName(this, "Play Movie", ".", "Citra TAS Movie (*.ctm)")};
    if (path.isEmpty())
        return;

    if (emulation_running) {
        if (!ValidateMovie(path)) {
            return;
        }
    } else {
        u64 program_id{Core::Movie::GetInstance().GetMovieProgramID(path.toStdString())};
        QString game_path{game_list->FindGameByProgramID(program_id)};
        if (game_path.isEmpty()) {
            const int num_recent_files{
                std::min(UISettings::values.recent_files.size(), max_recent_files_item)};
            for (int i{}; i < num_recent_files; i++) {
                QString path{actions_recent_files[i]->data().toString()};
                if (!path.isEmpty()) {
                    if (QFile::exists(path)) {
                        auto loader{Loader::GetLoader(path.toStdString())};
                        u64 program_id_file;
                        if (loader->ReadProgramId(program_id_file) ==
                                Loader::ResultStatus::Success &&
                            program_id_file == program_id) {
                            game_path = path;
                        }
                    }
                }
            }
            if (game_path.isEmpty()) {
                QMessageBox::warning(this, "Game Not Found",
                                     "The movie you are trying to play is from a game that is not "
                                     "in the game list and is not in the recent files. If you own "
                                     "the game, add the game folder to the game list or open the "
                                     "game and try to play the movie again.");
                return;
            }
        }
        if (!ValidateMovie(path, program_id)) {
            return;
        }
        Core::Movie::GetInstance().PrepareForPlayback(path.toStdString());
        BootGame(game_path);
    }
    Core::Movie::GetInstance().StartPlayback(path.toStdString(), [this] {
        QMetaObject::invokeMethod(this, "OnMoviePlaybackCompleted");
    });
    ui.action_Record_Movie->setEnabled(false);
    ui.action_Play_Movie->setEnabled(false);
    ui.action_Stop_Recording_Playback->setEnabled(true);
}

void GMainWindow::OnStopRecordingPlayback() {
    if (movie_record_on_start) {
        QMessageBox::information(this, "Record Movie", "Movie recording cancelled.");
        movie_record_on_start = false;
        movie_record_path.clear();
    } else {
        const bool was_recording{Core::Movie::GetInstance().IsRecordingInput()};
        Core::Movie::GetInstance().Shutdown();
        if (was_recording) {
            QMessageBox::information(this, "Movie Saved", "The movie is successfully saved.");
        }
    }
    ui.action_Record_Movie->setEnabled(true);
    ui.action_Play_Movie->setEnabled(true);
    ui.action_Stop_Recording_Playback->setEnabled(false);
}

void GMainWindow::OnCaptureScreenshot() {
    OnPauseGame();
    const QString path{
        QFileDialog::getSaveFileName(this, "Capture Screenshot", ".", "PNG Image (*.png)")};
    OnStartGame();
    if (path.isEmpty())
        return;
    screens->CaptureScreenshot(UISettings::values.screenshot_resolution_factor, path);
}

void GMainWindow::UpdateStatusBar() {
    if (emu_thread == nullptr) {
        status_bar_update_timer.stop();
        return;
    }

    auto results{Core::System::GetInstance().GetAndResetPerfStats()};

    if (Settings::values.use_frame_limit) {
        emu_speed_label->setText(QString("Speed: %1% / %2%")
                                     .arg(results.emulation_speed * 100.0, 0, 'f', 0)
                                     .arg(Settings::values.frame_limit));
    } else {
        emu_speed_label->setText(
            QString("Speed: %1%").arg(results.emulation_speed * 100.0, 0, 'f', 0));
    }
    game_fps_label->setText(QString("Game: %1 FPS").arg(results.game_fps, 0, 'f', 0));
    emu_frametime_label->setText(
        QString("Frame: %1 ms").arg(results.frametime * 1000.0, 0, 'f', 2));

    emu_speed_label->setVisible(true);
    game_fps_label->setVisible(true);
    emu_frametime_label->setVisible(true);
}

void GMainWindow::OnCoreError(Core::System::ResultStatus result, const std::string& details) {
    QString message, title, status_message;
    switch (result) {
    case Core::System::ResultStatus::ErrorSystemFiles: {
        const QString common_message{
            "%1 is missing. Please <a "
            "href='https://github.com/citra-valentin/citra/wiki/"
            "Dumping-System-Archives-from-a-3DS-Console/'>dump your system "
            "archives</a>.<br/>Continuing emulation may result in crashes and bugs."};

        if (!details.empty()) {
            message = common_message.arg(QString::fromStdString(details));
        } else {
            message = common_message.arg("A system archive");
        }

        title = "System Archive Not Found";
        status_message = "System Archive Missing";
        break;
    }

    case Core::System::ResultStatus::ShutdownRequested: {
        if (cheats_window != nullptr) {
            cheats_window->close();
        }
        break;
    }

    case Core::System::ResultStatus::FatalError: {
        message = "A fatal error occured. Check the log for details.<br/>Continuing emulation may "
                  "result in crashes and bugs.";
        title = "Fatal Error";
        status_message = "Fatal Error encountered";
        break;
    }

    default:
        UNREACHABLE_MSG("Unknown result status");
        break;
    }

    QMessageBox message_box{};
    message_box.setWindowTitle(title);
    message_box.setText(message);
    message_box.setIcon(QMessageBox::Icon::Critical);
    QPushButton* continue_button{message_box.addButton("Continue", QMessageBox::RejectRole)};
    QPushButton* abort_button{message_box.addButton("Abort", QMessageBox::AcceptRole)};
    if (result != Core::System::ResultStatus::ShutdownRequested)
        message_box.exec();

    if (result == Core::System::ResultStatus::ShutdownRequested ||
        message_box.clickedButton() == abort_button) {
        if (emu_thread) {
            ShutdownGame();
            auto& system{Core::System::GetInstance()};
            if (!system.file_path.empty()) {
                BootGame(QString::fromStdString(system.file_path));
                system.file_path.clear();
            }
        }
    } else {
        // Only show the message if the game is still running.
        if (emu_thread) {
            Core::System::GetInstance().SetRunning(true);
            message_label->setText(status_message);
            message_label->setVisible(true);
        }
    }
}

void GMainWindow::OnMenuAboutCitra() {
    AboutDialog about{this};
    about.exec();
}

bool GMainWindow::ConfirmClose() {
    if (emu_thread == nullptr)
        return true;

    QMessageBox::StandardButton answer{
        QMessageBox::question(this, "Citra", "Are you sure you want to close Citra?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)};
    return answer != QMessageBox::No;
}

void GMainWindow::closeEvent(QCloseEvent* event) {
    if (!ConfirmClose()) {
        event->ignore();
        return;
    }

    if (!ui.action_Fullscreen->isChecked()) {
        UISettings::values.geometry = saveGeometry();
        UISettings::values.screens_geometry = screens->saveGeometry();
    }
    UISettings::values.state = saveState();
    UISettings::values.fullscreen = ui.action_Fullscreen->isChecked();
    UISettings::values.show_filter_bar = ui.action_Show_Filter_Bar->isChecked();
    UISettings::values.show_status_bar = ui.action_Show_Status_Bar->isChecked();

    game_list->SaveInterfaceLayout();
    SaveHotkeys();

    // Shutdown session if the emu thread is active...
    if (emu_thread != nullptr)
        ShutdownGame();

    screens->close();
    multiplayer_state->Close();
    QWidget::closeEvent(event);
}

static bool IsSingleFileDropEvent(QDropEvent* event) {
    const QMimeData* mimeData{event->mimeData()};
    return mimeData->hasUrls() && mimeData->urls().length() == 1;
}

void GMainWindow::dropEvent(QDropEvent* event) {
    if (IsSingleFileDropEvent(event) && ConfirmChangeGame()) {
        const QMimeData* mimeData{event->mimeData()};
        QString filename{mimeData->urls().at(0).toLocalFile()};
        BootGame(filename);
    }
}

void GMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (IsSingleFileDropEvent(event)) {
        event->acceptProposedAction();
    }
}

void GMainWindow::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

bool GMainWindow::ConfirmChangeGame() {
    if (emu_thread == nullptr)
        return true;

    auto answer{QMessageBox::question(
        this, "Citra",
        "Are you sure you want to stop the emulation? Any unsaved progress will be lost.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No)};
    return answer != QMessageBox::No;
}

void GMainWindow::filterBarSetChecked(bool state) {
    ui.action_Show_Filter_Bar->setChecked(state);
    emit(OnToggleFilterBar());
}

void GMainWindow::UpdateUITheme() {
    QStringList theme_paths{default_theme_paths};
    if (UISettings::values.theme != UISettings::themes[0].second &&
        !UISettings::values.theme.isEmpty()) {
        const QString theme_uri{":" + UISettings::values.theme + "/style.qss"};
        QFile f{theme_uri};
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream ts{&f};
            qApp->setStyleSheet(ts.readAll());
            GMainWindow::setStyleSheet(ts.readAll());
        } else {
            LOG_ERROR(Frontend, "Unable to set style, stylesheet file not found");
        }
        theme_paths.append(QStringList{":/icons/default", ":/icons/" + UISettings::values.theme});
        QIcon::setThemeName(":/icons/" + UISettings::values.theme);
    } else {
        qApp->setStyleSheet("");
        GMainWindow::setStyleSheet("");
        theme_paths.append(QStringList{":/icons/default"});
        QIcon::setThemeName(":/icons/default");
    }
    QIcon::setThemeSearchPaths(theme_paths);
}

void GMainWindow::OnMoviePlaybackCompleted() {
    QMessageBox::information(this, "Playback Completed", "Movie playback completed.");
    ui.action_Record_Movie->setEnabled(true);
    ui.action_Play_Movie->setEnabled(true);
    ui.action_Stop_Recording_Playback->setEnabled(false);
}

void GMainWindow::SetupUIStrings() {
    if (game_title.isEmpty()) {
        setWindowTitle(
            QString("Citra Valentin %1-%2").arg(Common::g_scm_branch, Common::g_scm_desc));
    } else {
        setWindowTitle(QString("Citra Valentin %1-%2 | %3")
                           .arg(Common::g_scm_branch, Common::g_scm_desc, game_title));
    }
}

void GMainWindow::SyncMenuUISettings() {
    ui.action_Screen_Layout_Default->setChecked(Settings::values.layout_option ==
                                                Settings::LayoutOption::Default);
    ui.action_Screen_Layout_Single_Screen->setChecked(Settings::values.layout_option ==
                                                      Settings::LayoutOption::SingleScreen);
    ui.action_Screen_Layout_Medium_Screen->setChecked(Settings::values.layout_option ==
                                                      Settings::LayoutOption::MediumScreen);
    ui.action_Screen_Layout_Large_Screen->setChecked(Settings::values.layout_option ==
                                                     Settings::LayoutOption::LargeScreen);
    ui.action_Screen_Layout_Side_by_Side->setChecked(Settings::values.layout_option ==
                                                     Settings::LayoutOption::SideScreen);
    ui.action_Screen_Layout_Swap_Screens->setChecked(Settings::values.swap_screen);
}

#ifdef main
#undef main
#endif

int main(int argc, char* argv[]) {
    Common::DetachedTasks detached_tasks;

    // Init settings params
    QCoreApplication::setOrganizationName("Citra team");
    QCoreApplication::setApplicationName("Citra");

    QApplication app{argc, argv};

    // Qt changes the locale and causes issues in float conversion using std::to_string() when
    // generating shaders
    setlocale(LC_ALL, "C");

    GMainWindow main_window;

    // Register camera factories
    Camera::RegisterFactory("image", std::make_unique<Camera::StillImageCameraFactory>());
    Camera::RegisterFactory("qt", std::make_unique<Camera::QtMultimediaCameraFactory>());
    Camera::QtMultimediaCameraHandler::Init();
    LOG_INFO(Frontend, "Citra version: {}-{}", Common::g_scm_branch, Common::g_scm_desc);
    SDL_InitSubSystem(SDL_INIT_AUDIO);
#ifdef _WIN32
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
    main_window.show();
    int result{app.exec()};
#ifdef _WIN32
    WSACleanup();
#endif
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    detached_tasks.WaitForAllTasks();
    return result;
}
