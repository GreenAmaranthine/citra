// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <fmt/format.h>
#include "citra/cheats.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/memory.h"
#include "ui_cheats.h"

CheatDialog::CheatDialog(Core::System& system, QWidget* parent)
    : QDialog{parent}, ui{std::make_unique<Ui::CheatDialog>()}, system{system} {
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    ui->setupUi(this);
    ui->tableCheats->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableCheats->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableCheats->setColumnWidth(0, 57);
    ui->tableCheats->setColumnWidth(1, 250);
    ui->tableCheats->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->labelTitle->setText(
        QString("Title ID: %1")
            .arg(QString::fromStdString(
                fmt::format("{:016X}", system.Kernel().GetCurrentProcess()->codeset->program_id))));
    connect(ui->buttonClose, &QPushButton::clicked, this, &CheatDialog::OnClose);
    connect(ui->buttonNewCheat, &QPushButton::clicked, this, &CheatDialog::OnAddCheat);
    connect(ui->buttonSave, &QPushButton::clicked, this, &CheatDialog::OnSave);
    connect(ui->buttonDelete, &QPushButton::clicked, this, &CheatDialog::OnDelete);
    connect(ui->tableCheats, &QTableWidget::cellClicked, this, &CheatDialog::OnRowSelected);
    connect(ui->textLines, &QPlainTextEdit::textChanged, this, &CheatDialog::OnLinesChanged);
    LoadCheats();
}

CheatDialog::~CheatDialog() {}

void CheatDialog::UpdateTitleID() {
    system.CheatManager().RefreshCheats();
    ui->labelTitle->setText(
        QString("Title ID: %1")
            .arg(QString::fromStdString(
                fmt::format("{:016X}", system.Kernel().GetCurrentProcess()->codeset->program_id))));
    LoadCheats();
    ui->textLines->setEnabled(false);
    selection_changing = true;
    ui->textLines->clear();
    selection_changing = false;
}

void CheatDialog::LoadCheats() {
    cheats = CheatCore::GetCheatsFromFile(system);
    ui->tableCheats->setRowCount(static_cast<int>(cheats.size()));
    for (int i{}; i < static_cast<int>(cheats.size()); i++) {
        auto enabled{new QCheckBox()};
        enabled->setChecked(cheats[i].GetEnabled());
        enabled->setStyleSheet("margin-left:7px;");
        ui->tableCheats->setItem(i, 0, new QTableWidgetItem());
        ui->tableCheats->setCellWidget(i, 0, enabled);
        ui->tableCheats->setItem(i, 1,
                                 new QTableWidgetItem(QString::fromStdString(cheats[i].GetName())));
        enabled->setProperty("row", static_cast<int>(i));
        ui->tableCheats->setRowHeight(i, 23);
        connect(enabled, &QCheckBox::stateChanged, this, &CheatDialog::OnCheckChanged);
    }
}

void CheatDialog::OnSave() {
    bool error{};
    QString error_message{"The following cheats are empty:\n\n%1"};
    QStringList empty_cheat_names;
    for (auto& cheat : cheats)
        if (cheat.GetCheatLines().empty()) {
            empty_cheat_names.append(QString::fromStdString(cheat.GetName()));
            error = true;
        }
    if (error) {
        QMessageBox::critical(this, "Error", error_message.arg(empty_cheat_names.join('\n')));
        return;
    }
    system.CheatManager().Save(cheats);
}

void CheatDialog::OnClose() {
    close();
}

void CheatDialog::OnRowSelected(int row, int column) {
    selection_changing = true;
    if (row == -1) {
        ui->textLines->clear();
        current_row = -1;
        selection_changing = false;
        ui->textLines->setEnabled(false);
        return;
    }
    ui->textLines->setEnabled(true);
    const auto& current_cheat{cheats[row]};
    std::vector<std::string> lines;
    for (const auto& line : current_cheat.GetCheatLines())
        lines.push_back(line.cheat_line);
    ui->textLines->setPlainText(QString::fromStdString(Common::Join(lines, "\n")));
    current_row = row;
    selection_changing = false;
}

void CheatDialog::OnLinesChanged() {
    if (selection_changing)
        return;
    auto lines{ui->textLines->toPlainText()};
    std::vector<std::string> lines_vec;
    Common::SplitString(lines.toStdString(), '\n', lines_vec);
    std::vector<CheatCore::CheatLine> new_lines;
    for (const auto& line : lines_vec)
        new_lines.emplace_back(line);
    cheats[current_row].SetCheatLines(new_lines);
}

void CheatDialog::OnCheckChanged(int state) {
    auto checkbox{qobject_cast<QCheckBox*>(sender())};
    int row{static_cast<int>(checkbox->property("row").toInt())};
    cheats[row].SetEnabled(static_cast<bool>(state));
}

void CheatDialog::OnDelete() {
    auto selection_model{ui->tableCheats->selectionModel()};
    auto selected{selection_model->selectedRows()};
    std::vector<int> rows;
    for (int i{selected.count() - 1}; i >= 0; i--) {
        QModelIndex index{selected.at(i)};
        int row{index.row()};
        cheats.erase(cheats.begin() + row);
        rows.push_back(row);
    }
    for (int row : rows)
        ui->tableCheats->removeRow(row);
    ui->tableCheats->clearSelection();
    OnRowSelected(-1, -1);
}

void CheatDialog::OnAddCheat() {
    NewCheatDialog dialog{this};
    dialog.exec();
    if (!dialog.IsCheatValid())
        return;
    auto result{dialog.GetReturnValue()};
    cheats.push_back(result);
    int new_cheat_index{static_cast<int>(cheats.size() - 1)};
    auto enabled{new QCheckBox()};
    ui->tableCheats->setRowCount(static_cast<int>(cheats.size()));
    enabled->setCheckState(Qt::CheckState::Unchecked);
    enabled->setStyleSheet("margin-left:7px;");
    ui->tableCheats->setItem(new_cheat_index, 0, new QTableWidgetItem());
    ui->tableCheats->setCellWidget(new_cheat_index, 0, enabled);
    ui->tableCheats->setItem(
        new_cheat_index, 1,
        new QTableWidgetItem(QString::fromStdString(cheats[new_cheat_index].GetName())));
    ui->tableCheats->setRowHeight(new_cheat_index, 23);
    enabled->setProperty("row", new_cheat_index);
    connect(enabled, &QCheckBox::stateChanged, this, &CheatDialog::OnCheckChanged);
    ui->tableCheats->selectRow(new_cheat_index);
    OnRowSelected(new_cheat_index, 0);
}

NewCheatDialog::NewCheatDialog(QWidget* parent)
    : QDialog{parent, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint} {
    setWindowTitle("New Cheat");
    auto main_layout{new QVBoxLayout(this)};
    auto name_panel{new QHBoxLayout()};
    auto name_label{new QLabel()};
    name_block = new QLineEdit();
    name_label->setText("Name: ");
    name_panel->addWidget(name_label);
    name_panel->addWidget(name_block);
    auto button_box{new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)};
    connect(button_box, &QDialogButtonBox::accepted, this, [&]() {
        auto name{name_block->text().toStdString()};
        if (Common::Trim(name).length() > 0) {
            return_value = CheatCore::Cheat(name_block->text().toStdString());
            cheat_valid = true;
        }
        close();
    });
    connect(button_box, &QDialogButtonBox::rejected, this, [&]() { close(); });
    auto confirmation_panel{new QHBoxLayout()};
    confirmation_panel->addWidget(button_box);
    main_layout->addLayout(name_panel);
    main_layout->addLayout(confirmation_panel);
}

NewCheatDialog::~NewCheatDialog() {}
