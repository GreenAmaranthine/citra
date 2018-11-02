// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef __APPLE__
#include <QMessageBox>
#endif
#include <QColorDialog>
#include "citra/configuration/configure_graphics.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_graphics.h"
#include "video_core/renderer/renderer.h"
#include "video_core/video_core.h"

ConfigureGraphics::ConfigureGraphics(QWidget* parent)
    : QWidget{parent}, ui{std::make_unique<Ui::ConfigureGraphics>()} {
    ui->setupUi(this);
    LoadConfiguration();
    ui->frame_limit->setEnabled(Settings::values.use_frame_limit);
    connect(ui->toggle_frame_limit, &QCheckBox::stateChanged, ui->frame_limit,
            &QSpinBox::setEnabled);
    ui->layoutBox->setEnabled(!Settings::values.custom_layout);
    connect(ui->layout_bg, &QPushButton::clicked, this, [this] {
        QColor new_color{QColorDialog::getColor(bg_color, this)};
        if (new_color.isValid()) {
            bg_color = new_color;
            ui->layout_bg->setStyleSheet(
                QString("QPushButton { background-color: %1 }").arg(bg_color.name()));
        }
    });
    ui->hw_shaders_group->setEnabled(ui->toggle_hw_shaders->isChecked());
    connect(ui->toggle_hw_shaders, &QCheckBox::stateChanged, ui->hw_shaders_group,
            &QWidget::setEnabled);
#ifdef __APPLE__
    connect(ui->toggle_hw_shaders, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::Checked) {
            QMessageBox::warning(
                this, "Hardware Shaders Warning",
                "Hardware Shaders support is broken on macOS, and will cause graphical issues "
                "like showing a black screen.<br><br>The option is only there for "
                "test/development purposes. If you experience graphical issues with Hardware "
                "Shader, please turn it off.");
        }
    });
#endif
}

ConfigureGraphics::~ConfigureGraphics() {}

void ConfigureGraphics::LoadConfiguration() {
    ui->toggle_hw_shaders->setChecked(Settings::values.use_hw_shaders);
    ui->toggle_accurate_gs->setChecked(Settings::values.shaders_accurate_gs);
    ui->toggle_accurate_mul->setChecked(Settings::values.shaders_accurate_mul);
    ui->resolution_factor_combobox->setCurrentIndex(Settings::values.resolution_factor - 1);
    ui->toggle_frame_limit->setChecked(Settings::values.use_frame_limit);
    ui->frame_limit->setValue(Settings::values.frame_limit);
    ui->layout_combobox->setCurrentIndex(static_cast<int>(Settings::values.layout_option));
    ui->swap_screen->setChecked(Settings::values.swap_screen);
    bg_color.setRgbF(Settings::values.bg_red, Settings::values.bg_green, Settings::values.bg_blue);
    ui->layout_bg->setStyleSheet(
        QString("QPushButton { background-color: %1 }").arg(bg_color.name()));
    ui->enable_shadows->setChecked(Settings::values.enable_shadows);
    ui->screen_refresh_rate->setValue(Settings::values.screen_refresh_rate);
    ui->min_vertices_per_thread->setValue(Settings::values.min_vertices_per_thread);
}

void ConfigureGraphics::ApplyConfiguration() {
    Settings::values.use_hw_shaders = ui->toggle_hw_shaders->isChecked();
    Settings::values.shaders_accurate_gs = ui->toggle_accurate_gs->isChecked();
    Settings::values.shaders_accurate_mul = ui->toggle_accurate_mul->isChecked();
    Settings::values.resolution_factor =
        static_cast<u16>(ui->resolution_factor_combobox->currentIndex() + 1);
    Settings::values.use_frame_limit = ui->toggle_frame_limit->isChecked();
    Settings::values.frame_limit = ui->frame_limit->value();
    Settings::values.bg_red = bg_color.redF();
    Settings::values.bg_green = bg_color.greenF();
    Settings::values.bg_blue = bg_color.blueF();
    Settings::values.layout_option =
        static_cast<Settings::LayoutOption>(ui->layout_combobox->currentIndex());
    Settings::values.swap_screen = ui->swap_screen->isChecked();
    Settings::values.enable_shadows = ui->enable_shadows->isChecked();
    Settings::values.screen_refresh_rate = ui->screen_refresh_rate->value();
    Settings::values.min_vertices_per_thread = ui->min_vertices_per_thread->value();
}
