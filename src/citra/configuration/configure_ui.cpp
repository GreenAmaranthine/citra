// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra/configuration/configure_ui.h"
#include "citra/ui_settings.h"
#include "ui_configure_ui.h"

ConfigureUi::ConfigureUi(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureUi) {
    ui->setupUi(this);

    for (const auto& theme : UISettings::themes) {
        ui->theme_combobox->addItem(theme.first, theme.second);
    }

    setConfiguration();
}

ConfigureUi::~ConfigureUi() = default;

void ConfigureUi::setConfiguration() {
    ui->theme_combobox->setCurrentIndex(ui->theme_combobox->findData(UISettings::values.theme));
    ui->icon_size_combobox->setCurrentIndex(
        static_cast<int>(UISettings::values.game_list_icon_size));
    ui->row_1_text_combobox->setCurrentIndex(static_cast<int>(UISettings::values.game_list_row_1));
    ui->row_2_text_combobox->setCurrentIndex(static_cast<int>(UISettings::values.game_list_row_2) +
                                             1);
    ui->toggle_hide_no_icon->setChecked(UISettings::values.game_list_hide_no_icon);
}

void ConfigureUi::applyConfiguration() {
    UISettings::values.theme =
        ui->theme_combobox->itemData(ui->theme_combobox->currentIndex()).toString();
    UISettings::values.game_list_icon_size =
        static_cast<UISettings::GameListIconSize>(ui->icon_size_combobox->currentIndex());
    UISettings::values.game_list_row_1 =
        static_cast<UISettings::GameListText>(ui->row_1_text_combobox->currentIndex());
    UISettings::values.game_list_row_2 =
        static_cast<UISettings::GameListText>(ui->row_2_text_combobox->currentIndex() - 1);
    UISettings::values.game_list_hide_no_icon = ui->toggle_hide_no_icon->isChecked();
}
