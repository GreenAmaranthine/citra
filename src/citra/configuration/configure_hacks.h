// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Core {
class System;
} // namespace Core

namespace Ui {
class ConfigureHacks;
} // namespace Ui

class ConfigureHacks : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureHacks(QWidget* parent = nullptr);
    ~ConfigureHacks();

    void LoadConfiguration(Core::System& system);
    void ApplyConfiguration(Core::System& system);

private:
    std::unique_ptr<Ui::ConfigureHacks> ui;
};
