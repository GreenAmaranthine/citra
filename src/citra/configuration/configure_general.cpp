// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra/configuration/configure_general.h"
#include "citra/ui_settings.h"
#include "citra/util/console.h"
#include "common/file_util.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_general.h"

ConfigureGeneral::ConfigureGeneral(QWidget* parent)
    : QWidget{parent}, ui{std::make_unique<Ui::ConfigureGeneral>()} {
    ui->setupUi(this);
#ifndef _WIN32
    ui->toggle_console->setText("Enable logging to console");
    ui->toggle_console->setToolTip(QString());
#endif
    setConfiguration();
}

ConfigureGeneral::~ConfigureGeneral() {}

void ConfigureGeneral::setConfiguration() {
    ui->combobox_keyboard_mode->setCurrentIndex(static_cast<int>(Settings::values.keyboard_mode));
    ui->toggle_console->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->toggle_console->setChecked(UISettings::values.show_console);
    ui->log_filter_edit->setText(QString::fromStdString(Settings::values.log_filter));
}

void ConfigureGeneral::applyConfiguration() {
    Settings::values.keyboard_mode =
        static_cast<Settings::KeyboardMode>(ui->combobox_keyboard_mode->currentIndex());
    UISettings::values.show_console = ui->toggle_console->isChecked();
    Settings::values.log_filter = ui->log_filter_edit->text().toStdString();
    Util::ToggleConsole();
    Log::Filter filter;
    filter.ParseFilterString(Settings::values.log_filter);
    Log::SetGlobalFilter(filter);
}
