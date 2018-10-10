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

class QComboBox;
class QLineEdit;
class QWidget;

namespace Ui {
class CheatDialog;
class NewCheatDialog;
} // namespace Ui

struct FoundItem {
    QString address;
    QString value;
};

class CheatDialog : public QDialog {
    Q_OBJECT

public:
    explicit CheatDialog(QWidget* parent = nullptr);
    ~CheatDialog();
    void UpdateTitleID();

private:
    std::unique_ptr<Ui::CheatDialog> ui;
    int current_row{-1};
    bool selection_changing{};
    std::vector<CheatCore::Cheat> cheats;
    std::vector<FoundItem> previous_found;
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
    template <typename T>
    std::vector<FoundItem> FirstSearch(const T value, std::function<bool(int, int, int)> comparer);
    template <typename T>
    std::vector<FoundItem> NextSearch(const T value, std::function<bool(int, int, int)> comparer);
    bool Equals(int a, int b, int c);
    bool LessThan(int a, int b, int c);
    bool GreaterThan(int a, int b, int c);
    bool Between(int min, int max, int value);
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
    ModifyAddressDialog(QWidget* parent, const QString& address, int type, const QString& value);
    ~ModifyAddressDialog();

    QString return_value;

private:
    QLineEdit* address_block;
    QLineEdit* value_block;
    QComboBox* type_select;

    void OnOkClicked();
};
