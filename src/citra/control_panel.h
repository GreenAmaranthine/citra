// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

namespace Ui {
class ControlPanel;
} // namespace Ui

class ControlPanel : public QDialog {
    Q_OBJECT

public:
    explicit ControlPanel(QWidget* parent = nullptr);
    ~ControlPanel();
    void Update3D();

private:
    std::unique_ptr<Ui::ControlPanel> ui;
};
