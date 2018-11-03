﻿// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include "core/settings.h"

class QSettings;

class Config {
public:
    Config();
    ~Config();

    void Save();
    void RestoreDefaults();

    static const std::array<int, Settings::NativeButton::NumButtons> default_buttons;
    static const std::array<std::array<int, 5>, Settings::NativeAnalog::NumAnalogs> default_analogs;

private:
    void Load();

    std::unique_ptr<QSettings> qt_config;
};