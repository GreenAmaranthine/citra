// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra/configuration/configure_hacks.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_hacks.h"

ConfigureHacks::ConfigureHacks(QWidget* parent)
    : QWidget{parent}, ui{std::make_unique<Ui::ConfigureHacks>()} {
    ui->setupUi(this);
}

ConfigureHacks::~ConfigureHacks() {}

void ConfigureHacks::LoadConfiguration(Core::System& system) {
    ui->toggle_priority_boost->setChecked(Settings::values.priority_boost);
    ui->combo_ticks_mode->setCurrentIndex(static_cast<int>(Settings::values.ticks_mode));
    ui->spinbox_ticks->setValue(static_cast<int>(Settings::values.ticks));
    ui->spinbox_ticks->setEnabled(Settings::values.ticks_mode == Settings::TicksMode::Custom);
    ui->toggle_bos->setChecked(Settings::values.use_bos);
    ui->toggle_force_memory_mode_7->setChecked(Settings::values.force_memory_mode_7);
    ui->disable_mh_2xmsaa->setChecked(Settings::values.disable_mh_2xmsaa);
    bool powered_on{system.IsPoweredOn()};
    ui->toggle_priority_boost->setEnabled(!powered_on);
    ui->toggle_force_memory_mode_7->setEnabled(!powered_on);
    ui->disable_mh_2xmsaa->setEnabled(!powered_on);
    connect(ui->combo_ticks_mode, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [&](int index) { ui->spinbox_ticks->setEnabled(index == 2); });
}

void ConfigureHacks::ApplyConfiguration(Core::System& system) {
    Settings::values.priority_boost = ui->toggle_priority_boost->isChecked();
    Settings::values.ticks_mode =
        static_cast<Settings::TicksMode>(ui->combo_ticks_mode->currentIndex());
    Settings::values.ticks = static_cast<u64>(ui->spinbox_ticks->value());
    Settings::values.use_bos = ui->toggle_bos->isChecked();
    Settings::values.force_memory_mode_7 = ui->toggle_force_memory_mode_7->isChecked();
    Settings::values.disable_mh_2xmsaa = ui->disable_mh_2xmsaa->isChecked();
    if (system.IsPoweredOn())
        system.CPU().SyncSettings();
}
