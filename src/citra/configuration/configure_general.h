// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

class HotkeyRegistry;

namespace Ui {
class ConfigureGeneral;
} // namespace Ui

class ConfigureGeneral : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGeneral(QWidget* parent = nullptr);
    ~ConfigureGeneral();

    void ApplyConfiguration();
    void PopulateHotkeyList(const HotkeyRegistry& registry);

signals:
    void RestoreDefaultsRequested();

private:
    void LoadConfiguration();

    std::unique_ptr<Ui::ConfigureGeneral> ui;
};
