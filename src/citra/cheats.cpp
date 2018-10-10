// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
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
#include "core/hle/kernel/process.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/memory.h"
#include "ui_cheats.h"

CheatDialog::CheatDialog(QWidget* parent)
    : QDialog{parent}, ui{std::make_unique<Ui::CheatDialog>()} {
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    ui->setupUi(this);
    ui->tableCheats->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableCheats->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableCheats->setColumnWidth(0, 57);
    ui->tableCheats->setColumnWidth(1, 250);
    ui->tableCheats->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->labelTitle->setText(QString("Title ID: %1")
                                .arg(QString::fromStdString(fmt::format(
                                    "{:016X}", Kernel::g_current_process->codeset->program_id))));
    ui->lblTo->hide();
    ui->txtSearchTo->hide();
    ui->tableFound->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableFound->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->buttonClose, &QPushButton::clicked, this, &CheatDialog::OnClose);
    connect(ui->buttonNewCheat, &QPushButton::clicked, this, &CheatDialog::OnAddCheat);
    connect(ui->buttonSave, &QPushButton::clicked, this, &CheatDialog::OnSave);
    connect(ui->buttonDelete, &QPushButton::clicked, this, &CheatDialog::OnDelete);
    connect(ui->tableCheats, &QTableWidget::cellClicked, this, &CheatDialog::OnRowSelected);
    connect(ui->textLines, &QPlainTextEdit::textChanged, this, &CheatDialog::OnLinesChanged);
    connect(ui->btnNextScan, &QPushButton::clicked, this, [this] { OnScan(true); });
    connect(ui->btnFirstScan, &QPushButton::clicked, this, [this] { OnScan(false); });
    connect(ui->cbScanType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) { OnScanTypeChanged(index); });
    connect(ui->cbValueType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) { OnValueTypeChanged(index); });
    connect(ui->chkHex, &QCheckBox::clicked, this,
            [this](bool state) { OnHexCheckedChanged(state); });
    connect(ui->tableFound, &QTableWidget::doubleClicked, this, [&](QModelIndex i) {
        ModifyAddressDialog* dialog{new ModifyAddressDialog(
            this, ui->tableFound->item(i.row(), 0)->text(), ui->cbValueType->currentIndex(),
            ui->tableFound->item(i.row(), 1)->text())};
        dialog->exec();
        QString rv{dialog->return_value};
        ui->tableFound->item(i.row(), 1)->setText(rv);
    });

    LoadCheats();
}

CheatDialog::~CheatDialog() {}

void CheatDialog::UpdateTitleID() {
    CheatCore::RefreshCheats();
    ui->labelTitle->setText(QString("Title ID: %1")
                                .arg(QString::fromStdString(fmt::format(
                                    "{:016X}", Kernel::g_current_process->codeset->program_id))));
    LoadCheats();
    ui->textLines->setEnabled(false);
    selection_changing = true;
    ui->textLines->clear();
    selection_changing = false;
    ui->lblCount->setText("Count: 0");
    ui->tableFound->setRowCount(0);
    ui->txtSearch->clear();
    ui->chkHex->setChecked(false);
    ui->lblTo->hide();
    ui->txtSearchTo->clear();
    ui->cbValueType->setCurrentIndex(0);
    ui->cbScanType->setCurrentIndex(0);
    ui->chkNot->setChecked(false);
    ui->btnNextScan->setEnabled(false);
}

void CheatDialog::LoadCheats() {
    cheats = CheatCore::Engine::ReadFileContents();

    ui->tableCheats->setRowCount(static_cast<int>(cheats.size()));

    for (int i{}; i < static_cast<int>(cheats.size()); i++) {
        QCheckBox* enabled{new QCheckBox()};
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
    QStringList empty_cheat_names{};
    for (auto& cheat : cheats) {
        if (cheat.GetCheatLines().empty()) {
            empty_cheat_names.append(QString::fromStdString(cheat.GetName()));
            error = true;
        }
    }
    if (error) {
        QMessageBox::critical(this, "Error", error_message.arg(empty_cheat_names.join('\n')));
        return;
    }

    CheatCore::Engine::Save(cheats);
    CheatCore::RefreshCheats();
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

    std::vector<std::string> lines{};
    for (const auto& line : current_cheat.GetCheatLines())
        lines.push_back(line.cheat_line);
    ui->textLines->setPlainText(QString::fromStdString(Common::Join(lines, "\n")));

    current_row = row;
    selection_changing = false;
}

void CheatDialog::OnLinesChanged() {
    if (selection_changing)
        return;
    QString lines{ui->textLines->toPlainText()};
    std::vector<std::string> lines_vec{};
    Common::SplitString(lines.toStdString(), '\n', lines_vec);
    auto new_lines{std::vector<CheatCore::CheatLine>()};
    for (const auto& line : lines_vec) {
        new_lines.emplace_back(line);
    }
    cheats[current_row].SetCheatLines(new_lines);
}

void CheatDialog::OnCheckChanged(int state) {
    QCheckBox* checkbox{qobject_cast<QCheckBox*>(sender())};
    int row{static_cast<int>(checkbox->property("row").toInt())};
    cheats[row].SetEnabled(static_cast<bool>(state));
}

void CheatDialog::OnDelete() {
    QItemSelectionModel* selectionModel{ui->tableCheats->selectionModel()};
    QModelIndexList selected{selectionModel->selectedRows()};
    std::vector<int> rows{};
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

    if (!dialog.IsCheatValid()) {
        return;
    }
    auto result{dialog.GetReturnValue()};
    cheats.push_back(result);
    int new_cheat_index{static_cast<int>(cheats.size() - 1)};
    QCheckBox* enabled{new QCheckBox()};
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

    QVBoxLayout* main_layout{new QVBoxLayout(this)};
    QHBoxLayout* name_panel{new QHBoxLayout()};
    QLabel* name_label{new QLabel()};
    name_block = new QLineEdit();
    name_label->setText("Name: ");
    name_panel->addWidget(name_label);
    name_panel->addWidget(name_block);

    QDialogButtonBox* button_box{
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)};
    connect(button_box, &QDialogButtonBox::accepted, this, [&]() {
        std::string name{name_block->text().toStdString()};
        if (Common::Trim(name).length() > 0) {
            return_value = CheatCore::Cheat(name_block->text().toStdString());
            cheat_valid = true;
        }
        close();
    });
    connect(button_box, &QDialogButtonBox::rejected, this, [&]() { close(); });
    QHBoxLayout* confirmation_panel{new QHBoxLayout()};
    confirmation_panel->addWidget(button_box);
    main_layout->addLayout(name_panel);
    main_layout->addLayout(confirmation_panel);
}

NewCheatDialog::~NewCheatDialog() {}

template <typename T>
T Read(const VAddr addr) {
    if (std::is_same<T, u8>::value) {
        return Memory::Read8(addr);
    } else if (std::is_same<T, u16>::value) {
        return Memory::Read16(addr);
    } else if (std::is_same<T, u32>::value) {
        return Memory::Read32(addr);
    }
}

QString IntToHex(int value) {
    std::stringstream ss{};
    ss << std::setfill('0') << std::setw(sizeof(int) * 2) << std::hex << value;
    return QString::fromStdString(ss.str());
}

int HexToInt(const QString& hex) {
    int dec{};
    std::stringstream ss{};
    ss << hex.toStdString();
    ss >> std::hex >> dec;
    return dec;
}

double HexStringToDouble(const QString& hex) {
    union {
        long long i;
        double d;
    } value;

    value.i = std::stoll(hex.toStdString(), nullptr, 16);

    return value.d;
}

QString DoubleToHexString(double value) {
    union {
        long long i;
        double d;
    };

    d = value;

    std::ostringstream oss{};
    oss << std::hex << std::setfill('0') << std::setw(16) << i;

    return QString::fromStdString(oss.str());
}

QString IeeeFloatToHex(float value) {
    union {
        float f;
        u32 i;
    };

    f = value;

    std::ostringstream oss{};
    oss << std::hex << std::uppercase << i;

    return QString::fromStdString(oss.str());
}

void CheatDialog::OnScan(bool is_next_scan) {
    int value_type{};
    int search_type{};
    QString search_value{};
    bool convert_hex{};

    try {
        value_type = ui->cbValueType->currentIndex();
        search_type = ui->cbScanType->currentIndex();
        search_value = ui->txtSearch->text();
        convert_hex = ui->chkHex->isChecked();
    } catch (const std::exception& e) {
        LOG_CRITICAL(Frontend, "Exception when scanning: {}", e.what());
        ui->txtSearch->clear();
        return;
    }

    std::function<bool(int, int, int)> comparer{};

    switch (search_type) {
    case 0: { // Equals
        comparer = [&](int a, int b, int c) { return Equals(a, b, c); };
        break;
    }
    case 1: { // Greater Than
        comparer = [&](int a, int b, int c) { return GreaterThan(a, b, c); };
        break;
    }
    case 2: { // Less Than
        comparer = [&](int a, int b, int c) { return LessThan(a, b, c); };
        break;
    }
    case 3: { // Between
        comparer = [&](int a, int b, int c) { return Between(a, b, c); };
        break;
    }
    }

    int base{ui->chkHex->isChecked() ? 16 : 10};

    switch (value_type) {
    case 0: { // u32
        u32 value{search_value.toUInt(nullptr, base)};
        if (!is_next_scan)
            previous_found = FirstSearch<u32>(value, comparer);
        else
            previous_found = NextSearch<u32>(value, comparer);
        break;
    }

    case 1: { // u16
        u16 value{search_value.toUShort(nullptr, base)};
        if (!is_next_scan)
            previous_found = FirstSearch<u16>(value, comparer);
        else
            previous_found = NextSearch<u16>(value, comparer);
        break;
    }
    case 2: { // u8
        u8 value{static_cast<u8>(search_value.toUShort(nullptr, base))};
        if (!is_next_scan)
            previous_found = FirstSearch<u8>(value, comparer);
        else
            previous_found = NextSearch<u8>(value, comparer);
        break;
    }
    }

    ui->tableFound->setRowCount(0);

    if (previous_found.size() > 50000) {
        ui->lblCount->setText("Count: 50000+");
    } else {
        LoadTable(previous_found);
        ui->lblCount->setText(QString("Count: %1").arg(previous_found.size()));
    }

    ui->btnNextScan->setEnabled(previous_found.size() > 0);
}

void CheatDialog::OnValueTypeChanged(int index) {
    ui->txtSearch->clear();
    ui->txtSearchTo->clear();
    if (index >= 0 && index <= 2) {
        ui->chkHex->setVisible(true);
    } else {
        ui->chkHex->setVisible(false);
        ui->chkHex->setChecked(false);
    }
}

void CheatDialog::OnScanTypeChanged(int index) {
    if (index == 3) { // Between
        ui->lblTo->setVisible(true);
        ui->txtSearchTo->setVisible(true);
    } else {
        ui->lblTo->setVisible(false);
        ui->txtSearchTo->setVisible(false);
        ui->txtSearchTo->clear();
    }

    if (index == 0) { // Equals
        ui->chkNot->setVisible(true);
    } else {
        ui->chkNot->setVisible(false);
        ui->chkNot->setChecked(false);
    }
}

void CheatDialog::OnHexCheckedChanged(bool checked) {
    QString text{ui->txtSearch->text()};
    QString text_to{ui->txtSearchTo->text()};

    try {
        if (checked) {
            if (text.length() > 0) {
                int val{std::stoi(text.toStdString(), nullptr, 10)};
                ui->txtSearch->setText(IntToHex(val));
            }

            if (text_to.length() > 0) {
                int val2{std::stoi(text_to.toStdString(), nullptr, 10)};
                ui->txtSearchTo->setText(IntToHex(val2));
            }
        } else {
            if (text.length() > 0) {
                ui->txtSearch->setText(QString::number(HexToInt(text)));
            }

            if (text_to.length() > 0) {
                ui->txtSearchTo->setText(QString::number(HexToInt(text_to)));
            }
        }
    } catch (const std::exception&) {
        ui->txtSearch->clear();
        ui->txtSearchTo->clear();
    }
}

void CheatDialog::LoadTable(const std::vector<FoundItem>& items) {
    ui->tableFound->setRowCount(static_cast<int>(items.size()));

    for (int i{}; i < items.size(); i++) {
        ui->tableFound->setItem(i, 0, new QTableWidgetItem(items[i].address.toUpper()));
        ui->tableFound->setItem(i, 1, new QTableWidgetItem(items[i].value));
        ui->tableFound->setRowHeight(i, 23);
    }
}

template <typename T>
std::vector<FoundItem> CheatDialog::FirstSearch(const T value,
                                                std::function<bool(int, int, int)> comparer) {
    std::vector<VAddr> address_in_use{};
    std::vector<FoundItem> results{};
    int base{ui->chkHex->isChecked() ? 16 : 10};
    T search_to_value{static_cast<T>(ui->txtSearchTo->text().toInt(nullptr, base))};

    for (VAddr i{Memory::HEAP_VADDR}; i < Memory::HEAP_VADDR_END; i += 4096) {
        if (Memory::IsValidVirtualAddress(i)) {
            address_in_use.push_back(i);
        }
    }

    for (VAddr i{Memory::LINEAR_HEAP_VADDR}; i < Memory::LINEAR_HEAP_VADDR_END; i += 4096) {
        if (Memory::IsValidVirtualAddress(i)) {
            address_in_use.push_back(i);
        }
    }

    if (Service::CFG::IsNewModeEnabled()) {
        for (VAddr i{Memory::NEW_LINEAR_HEAP_VADDR}; i < Memory::NEW_LINEAR_HEAP_VADDR_END;
             i += 4096) {
            if (Memory::IsValidVirtualAddress(i)) {
                address_in_use.push_back(i);
            }
        }
    }

    for (auto& range : address_in_use) {
        for (VAddr i{range}; i < (range + 4096); i++) {
            T result{Read<T>(i)};
            if (comparer(result, value, search_to_value)) {
                FoundItem item{};
                item.address = IntToHex(i);
                item.value = QString::number(result);
                results.push_back(item);
            }
        }
    }
    return results;
}

template <typename T>
std::vector<FoundItem> CheatDialog::NextSearch(const T value,
                                               std::function<bool(int, int, int)> comparer) {
    std::vector<FoundItem> results{};
    int base{ui->chkHex->isChecked() ? 16 : 10};
    T search_to_value{static_cast<T>(ui->txtSearchTo->text().toUInt(nullptr, base))};

    for (auto& f : previous_found) {
        VAddr addr{static_cast<VAddr>(f.address.toUInt(nullptr, 16))};
        T result{Read<T>(addr)};

        if (comparer(result, value, search_to_value)) {
            FoundItem item{};
            item.address = IntToHex(addr);
            item.value = QString::number(result);
            results.push_back(item);
        }
    }

    return results;
}

bool CheatDialog::Equals(int a, int b, int c) {
    return ui->chkNot->isChecked() ? (a != b) : (a == b);
}

bool CheatDialog::LessThan(int a, int b, int c) {
    return a < b;
}

bool CheatDialog::GreaterThan(int a, int b, int c) {
    return a > b;
}

bool CheatDialog::Between(int value, int min, int max) {
    return min < value && value < max;
}

ModifyAddressDialog::ModifyAddressDialog(QWidget* parent, const QString& address, int type,
                                         const QString& value)
    : QDialog{parent, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint} {
    setWindowTitle("Modify Address");
    setSizeGripEnabled(false);

    QVBoxLayout* main_layout{new QVBoxLayout(this)};

    QHBoxLayout* edit_panel{new QHBoxLayout()};
    address_block = new QLineEdit();
    value_block = new QLineEdit();
    type_select = new QComboBox();

    address_block->setReadOnly(true);
    address_block->setText(address);

    type_select->addItem("u32");
    type_select->addItem("u16");
    type_select->addItem("u8");
    type_select->addItem("float");
    type_select->addItem("double");
    type_select->setCurrentIndex(type);

    value_block->setText(value);

    edit_panel->addWidget(address_block);
    edit_panel->addWidget(type_select);
    edit_panel->addWidget(value_block);

    auto button_box{new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)};
    connect(button_box, &QDialogButtonBox::accepted, this, [&] { OnOkClicked(); });
    connect(button_box, &QDialogButtonBox::rejected, this, [&] {
        return_value = value;
        close();
    });
    QHBoxLayout* confirmation_panel{new QHBoxLayout()};
    confirmation_panel->addWidget(button_box);
    main_layout->addLayout(edit_panel);
    main_layout->addLayout(confirmation_panel);
}

ModifyAddressDialog::~ModifyAddressDialog() {}

void ModifyAddressDialog::OnOkClicked() {
    int value_type{};
    QString new_value{};
    int address{};

    try {
        value_type = type_select->currentIndex();
        new_value = value_block->text();
        address = address_block->text().toInt(nullptr, 16);
    } catch (const std::exception&) {
        close();
    }

    switch (value_type) {
    case 0: { // u32
        u32 value{new_value.toUInt(nullptr, 10)};
        Memory::Write32(address, value);
        Core::GetCPU().InvalidateCacheRange(address, sizeof(u32));
        break;
    }
    case 1: { // u16
        u16 value{new_value.toUShort(nullptr, 10)};
        Memory::Write16(address, value);
        Core::GetCPU().InvalidateCacheRange(address, sizeof(u16));
        break;
    }
    case 2: { // u8
        u8 value{static_cast<u8>(new_value.toUShort(nullptr, 10))};
        Memory::Write8(address, value);
        Core::GetCPU().InvalidateCacheRange(address, sizeof(u8));
        break;
    }
    case 3: { // float
        float value{new_value.toFloat()};
        u32 converted{IeeeFloatToHex(value).toUInt(nullptr, 16)};
        Memory::Write32(address, converted);
        Core::GetCPU().InvalidateCacheRange(address, sizeof(u32));
        break;
    }
    case 4: { // double
        double value{new_value.toDouble()};
        u64 converted{DoubleToHexString(value).toULongLong(nullptr, 10)};
        Memory::Write64(address, converted);
        Core::GetCPU().InvalidateCacheRange(address, sizeof(u64));
        break;
    }
    }

    return_value = new_value;
    close();
}
