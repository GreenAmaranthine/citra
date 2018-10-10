// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

namespace Ui {
class ConfigurationDialog;
} // namespace Ui

class ConfigurationDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigurationDialog(QWidget* parent);
    ~ConfigurationDialog();

    void applyConfiguration();

private:
    std::unique_ptr<Ui::ConfigurationDialog> ui;
};
