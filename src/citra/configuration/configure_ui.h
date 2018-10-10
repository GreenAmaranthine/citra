// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureUi;
} // namespace Ui

class ConfigureUi : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureUi(QWidget* parent = nullptr);
    ~ConfigureUi();

    void applyConfiguration();

private:
    void setConfiguration();

    std::unique_ptr<Ui::ConfigureUi> ui;
};
