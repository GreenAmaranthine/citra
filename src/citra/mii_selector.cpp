// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string>
#include <QMessageBox>
#include "citra/mii_selector.h"
#include "common/file_util.h"
#include "core/file_sys/archive_extsavedata.h"
#include "core/file_sys/file_backend.h"
#include "core/hle/service/ptm/ptm.h"
#include "ui_mii_selector.h"

u16 crc16_ccitt(const u8* buf, std::size_t len, u32 starting_val) {
    if (!buf) {
        return -1;
    }

    u32 crc{starting_val};

    static const u16 POLY{0x1021};

    for (std::size_t i{}; i < len; i++) {
        for (int bit{7}; bit >= 0; bit--) {
            crc = ((crc << 1) | ((buf[i] >> bit) & 0x1)) ^ (crc & 0x8000 ? POLY : 0);
        }
    }

    for (int _{}; _ < 16; _++) {
        crc = (crc << 1) ^ (crc & 0x8000 ? POLY : 0);
    }

    return (u16)(crc & 0xffff);
}

MiiSelectorDialog::MiiSelectorDialog(QWidget* parent, const HLE::Applets::MiiConfig& config,
                                     HLE::Applets::MiiResult& result)
    : QDialog{parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint},
      ui{std::make_unique<Ui::MiiSelectorDialog>()} {
    ui->setupUi(this);

    if (config.magic_value != 0x13DE28CF) {
        result.return_code = 1;
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return;
    }

    ui->cancel->setEnabled(config.enable_cancel_button == 1);
    std::u16string title{reinterpret_cast<const char16_t*>(config.title.data())};
    setWindowTitle(title.empty() ? "Mii Selector" : QString::fromStdU16String(title));

    std::string nand_directory{FileUtil::GetUserPath(D_NAND_IDX)};
    FileSys::ArchiveFactory_ExtSaveData extdata_archive_factory{nand_directory, true};

    auto archive_result{extdata_archive_factory.Open(Service::PTM::ptm_shared_extdata_id)};
    if (!archive_result.Succeeded()) {
        ShowNoMiis(result);
        return;
    }

    auto archive{std::move(archive_result).Unwrap()};

    FileSys::Path file_path{"/CFL_DB.dat"};
    FileSys::Mode mode{};
    mode.read_flag.Assign(1);

    auto file_result{archive->OpenFile(file_path, mode)};
    if (!file_result.Succeeded()) {
        ShowNoMiis(result);
        return;
    }

    auto file{std::move(file_result).Unwrap()};

    u32 id;
    u32 offset{0x8};
    std::array<u8, 0x5C> mii;

    for (int i{}; i < 100; ++i) {
        file->Read(offset, 0x5C, mii.data());
        std::memcpy(&id, mii.data(), sizeof(u32));
        if (id != 0) {
            std::u16string name(10, '\0');
            std::memcpy(&name[0], mii.data() + 0x1A, 0x14);
            miis.emplace(ui->mii->count(), mii);
            ui->mii->addItem(QString::fromStdU16String(name));
        }
        offset += 0x5C;
    }

    if (miis.empty()) {
        ShowNoMiis(result);
        return;
    }

    if (static_cast<int>(config.initially_selected_mii_index) != 0xFFFF) {
        ui->mii->setCurrentIndex(static_cast<int>(config.initially_selected_mii_index));
    }

    connect(ui->ok, &QPushButton::released, this, [&] {
        std::memcpy(result.selected_mii_data, miis.at(ui->mii->currentIndex()).data(), 0x5C);
        result.mii_data_checksum =
            crc16_ccitt(result.selected_mii_data, 0x5C + sizeof(result.pad49), 0);
        result.selected_guest_mii_index = 0xFFFFFFFF;
        close();
    });

    connect(ui->cancel, &QPushButton::released, this, [&] {
        result.return_code = 1;
        close();
    });
}

MiiSelectorDialog::~MiiSelectorDialog() {}

void MiiSelectorDialog::ShowNoMiis(HLE::Applets::MiiResult& result) {
    QMessageBox::critical(nullptr, "Mii Selector",
                          "You don't have Miis.\nCreate a Mii with Mii Maker.");
    result.return_code = 1;
    QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}
