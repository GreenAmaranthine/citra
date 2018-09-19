// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <SDL.h>
#include "audio_core/sink.h"
#include "citra_qt/configuration/configure_audio.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_audio.h"

ConfigureAudio::ConfigureAudio(QWidget* parent)
    : QWidget{parent}, ui{std::make_unique<Ui::ConfigureAudio>()} {
    ui->setupUi(this);

    setConfiguration();
}

ConfigureAudio::~ConfigureAudio() {}

void ConfigureAudio::setConfiguration() {
    // Load output devices
    {
        ui->output_device_combo_box->addItem("auto");
        std::vector<std::string> device_list{AudioCore::ListDevices()};
        for (const auto& device : device_list) {
            ui->output_device_combo_box->addItem(device.c_str());
        }

        int new_device_index{-1};
        for (int index{}; index < ui->output_device_combo_box->count(); index++) {
            if (ui->output_device_combo_box->itemText(index).toStdString() ==
                Settings::values.output_device) {
                new_device_index = index;
                break;
            }
        }

        ui->output_device_combo_box->setCurrentIndex(new_device_index);
    }

    // Load input devices
    {
        ui->input_device_combo_box->addItem("auto");
        int num_devices{SDL_GetNumAudioDevices(1)};
        for (int i{}; i < num_devices; ++i) {
            ui->input_device_combo_box->addItem(SDL_GetAudioDeviceName(i, 1));
        }

        int new_device_index{-1};
        for (int index{}; index < ui->input_device_combo_box->count(); index++) {
            if (ui->input_device_combo_box->itemText(index).toStdString() ==
                Settings::values.input_device) {
                new_device_index = index;
                break;
            }
        }

        ui->input_device_combo_box->setCurrentIndex(new_device_index);
    }
}

void ConfigureAudio::applyConfiguration() {
    Settings::values.output_device = ui->output_device_combo_box->currentText().toStdString();
    Settings::values.input_device = ui->input_device_combo_box->currentText().toStdString();
}
