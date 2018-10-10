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
}

ConfigurationDialog::~ConfigurationDialog() {}

void ConfigurationDialog::applyConfiguration() {
    ui->generalTab->applyConfiguration();
    ui->systemTab->applyConfiguration();
    ui->inputTab->applyConfiguration();
    ui->inputTab->applyProfile();
    ui->graphicsTab->applyConfiguration();
    ui->audioTab->applyConfiguration();
    ui->cameraTab->applyConfiguration();
    ui->webTab->applyConfiguration();
    ui->hacksTab->applyConfiguration();
    ui->uiTab->applyConfiguration();
    Settings::Apply();
    Settings::LogSettings();
}
