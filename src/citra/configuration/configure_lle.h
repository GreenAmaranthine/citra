// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Core {
class System;
} // namespace Core

namespace Ui {
class ConfigureLLE;
} // namespace Ui

template <typename>
class QList;
class QCheckBox;

class ConfigureLLE : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureLLE(QWidget* parent = nullptr);
    ~ConfigureLLE();

    void LoadConfiguration(Core::System& system);
    void ApplyConfiguration();

private:
    std::unique_ptr<Ui::ConfigureLLE> ui;
    QList<QCheckBox*> module_checkboxes;
};
