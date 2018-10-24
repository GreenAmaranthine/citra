// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <QVariant>
#include "core/settings.h"

class QSettings;

class Config {
public:
    Config();
    ~Config();

    void Reload();
    void Save();

    static const std::array<int, Settings::NativeButton::NumButtons> default_buttons;
    static const std::array<std::array<int, 5>, Settings::NativeAnalog::NumAnalogs> default_analogs;

private:
    QSettings* qt_config;

    void ReadValues();
    void SaveValues();
};
