// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include "core/cheat_core.h"

namespace Core {
class System;
} // namespace Core

namespace Ui {
class CheatDialog;
class NewCheatDialog;
} // namespace Ui

class QComboBox;
class QLineEdit;
class QWidget;

class CheatDialog : public QDialog {
    Q_OBJECT

public:
    explicit CheatDialog(Core::System& system, QWidget* parent = nullptr);
    ~CheatDialog();

    void UpdateTitleID();

private:
    void LoadCheats();
    void OnAddCheat();
    void OnSave();
    void OnClose();
    void OnRowSelected(int row, int column);
    void OnLinesChanged();
    void OnCheckChanged(int state);
    void OnDelete();

    std::unique_ptr<Ui::CheatDialog> ui;
    int current_row{-1};
    bool selection_changing{};
    std::vector<CheatCore::Cheat> cheats;

    Core::System& system;
};

class NewCheatDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewCheatDialog(QWidget* parent = nullptr);
    ~NewCheatDialog();

    CheatCore::Cheat GetReturnValue() const {
        return return_value;
    }

    bool IsCheatValid() const {
        return cheat_valid;
    }

private:
    bool cheat_valid{};
    QLineEdit* name_block;
    QComboBox* type_select;
    CheatCore::Cheat return_value;
};
