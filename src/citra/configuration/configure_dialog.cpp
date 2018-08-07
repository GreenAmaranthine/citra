// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QHash>
#include <QListWidgetItem>
#include "citra/configuration/config.h"
#include "citra/configuration/configure_dialog.h"
#include "core/settings.h"
#include "citra/hotkeys.h"
#include "ui_configure.h"

ConfigurationDialog::ConfigurationDialog(QWidget* parent, const HotkeyRegistry& registry)
    : QDialog{parent}, ui{std::make_unique<Ui::ConfigurationDialog>()} {
    ui->setupUi(this);
    ui->generalTab->PopulateHotkeyList(registry);
    PopulateSelectionList();
    connect(ui->generalTab, &ConfigureGeneral::RestoreDefaultsRequested, [this] {
        restore_defaults_requested = true;
        accept();
    });
    connect(ui->selectorList, &QListWidget::itemSelectionChanged, this,
            &ConfigurationDialog::UpdateVisibleTabs);
    adjustSize();
    ui->selectorList->setCurrentRow(0);
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

void ConfigurationDialog::PopulateSelectionList() {
    const std::array<std::pair<QString, QStringList>, 4> items{{
        {"General", {"General", "Web", "Hacks", "UI"}},
        {"System", {"System", "Audio", "Camera"}},
        {"Graphics", {"Graphics"}},
        {"Controls", {"Input"}},
    }};
    for (const auto& entry : items) {
        auto* item{new QListWidgetItem(entry.first)};
        item->setData(Qt::UserRole, entry.second);
        ui->selectorList->addItem(item);
    }
}

void ConfigurationDialog::UpdateVisibleTabs() {
    auto items{ui->selectorList->selectedItems()};
    if (items.isEmpty())
        return;
    const QHash<QString, QWidget*> widgets{{
        {"General", ui->generalTab},
        {"System", ui->systemTab},
        {"Input", ui->inputTab},
        {"Graphics", ui->graphicsTab},
        {"Audio", ui->audioTab},
        {"Camera", ui->cameraTab},
        {"Camera", ui->cameraTab},
        {"Hacks", ui->hacksTab},
        {"Web", ui->webTab},
        {"UI", ui->uiTab},
    }};
    ui->tabWidget->clear();
    QStringList tabs{items[0]->data(Qt::UserRole).toStringList()};
    for (const auto& tab : tabs)
        ui->tabWidget->addTab(widgets[tab], tab);
}
