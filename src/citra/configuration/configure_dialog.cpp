// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra/configuration/config.h"
#include "citra/configuration/configure_dialog.h"
#include "core/settings.h"
#include "ui_configure.h"

ConfigurationDialog::ConfigurationDialog(QWidget* parent)
    : QDialog{parent}, ui{std::make_unique<Ui::ConfigurationDialog>()} {
    ui->setupUi(this);
    connect(ui->generalTab, &ConfigureGeneral::RestoreDefaultsRequested, [this] {
        restore_defaults_requested = true;
        accept();
    });
}

ConfigurationDialog::~ConfigurationDialog() {}

void ConfigurationDialog::ApplyConfiguration() {
    ui->generalTab->ApplyConfiguration();
    ui->systemTab->ApplyConfiguration();
    ui->inputTab->ApplyConfiguration();
    ui->inputTab->ApplyProfile();
    ui->graphicsTab->ApplyConfiguration();
    ui->audioTab->ApplyConfiguration();
    ui->cameraTab->ApplyConfiguration();
    ui->webTab->ApplyConfiguration();
    ui->hacksTab->ApplyConfiguration();
    ui->uiTab->ApplyConfiguration();
    Settings::Apply();
    Settings::LogSettings();
}
