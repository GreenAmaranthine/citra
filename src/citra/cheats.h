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
}

namespace Ui {
class CheatDialog;
class NewCheatDialog;
} // namespace Ui

class QComboBox;
class QLineEdit;
class QWidget;

struct FoundItem {
    QString address;
    QString value;
};

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
    void OnScan(bool is_next_scan);
    void OnScanTypeChanged(int index);
    void OnValueTypeChanged(int index);
    void OnHexCheckedChanged(bool checked);
    void LoadTable(const std::vector<FoundItem>& items);
    bool Equals(int a, int b, int c);
    bool LessThan(int a, int b, int c);
    bool GreaterThan(int a, int b, int c);
    bool Between(int min, int max, int value);

    template <typename T>
    std::vector<FoundItem> FirstSearch(const T value, std::function<bool(int, int, int)> comparer);

    template <typename T>
    std::vector<FoundItem> NextSearch(const T value, std::function<bool(int, int, int)> comparer);

    std::unique_ptr<Ui::CheatDialog> ui;
    int current_row{-1};
    bool selection_changing{};
    std::vector<CheatCore::Cheat> cheats;
    std::vector<FoundItem> previous_found;

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

class ModifyAddressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ModifyAddressDialog(Core::System& system, QWidget* parent, const QString& address,
                                 int type, const QString& value);
    ~ModifyAddressDialog();

    QString GetReturnValue() const {
        return return_value;
    }

private:
    void OnOkClicked();

    QLineEdit* address_block;
    QLineEdit* value_block;
    QComboBox* type_select;
    QString return_value;
    Core::System& system;
};
