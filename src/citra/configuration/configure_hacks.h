// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureHacks;
} // namespace Ui

class ConfigureHacks : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureHacks(QWidget* parent = nullptr);
    ~ConfigureHacks();

    void ApplyConfiguration();

private:
    void LoadConfiguration();
    std::unique_ptr<Ui::ConfigureHacks> ui;
};
