// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QMessageBox>
#include "citra/configuration/configure_system.h"
#include "citra/ui_settings.h"
#include "core/core.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/hle/service/fs/archive.h"
#include "core/settings.h"
#include "ui_configure_system.h"

static const std::array<int, 12> days_in_month{{
    31,
    29,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31,
}};

static const std::array<const char*, 187> country_names{{
    "",
    "Japan",
    "",
    "",
    "",
    "",
    "",
    "",
    "Anguilla",
    "Antigua and Barbuda", // 0-9
    "Argentina",
    "Aruba",
    "Bahamas",
    "Barbados",
    "Belize",
    "Bolivia",
    "Brazil",
    "British Virgin Islands",
    "Canada",
    "Cayman Islands", // 10-19
    "Chile",
    "Colombia",
    "Costa Rica",
    "Dominica",
    "Dominican Republic",
    "Ecuador",
    "El Salvador",
    "French Guiana",
    "Grenada",
    "Guadeloupe", // 20-29
    "Guatemala",
    "Guyana",
    "Haiti",
    "Honduras",
    "Jamaica",
    "Martinique",
    "Mexico",
    "Montserrat",
    "Netherlands Antilles",
    "Nicaragua", // 30-39
    "Panama",
    "Paraguay",
    "Peru",
    "Saint Kitts and Nevis",
    "Saint Lucia",
    "Saint Vincent and the Grenadines",
    "Suriname",
    "Trinidad and Tobago",
    "Turks and Caicos Islands",
    "United States", // 40-49
    "Uruguay",
    "US Virgin Islands",
    "Venezuela",
    "",
    "",
    "",
    "",
    "",
    "",
    "", // 50-59
    "",
    "",
    "",
    "",
    "Albania",
    "Australia",
    "Austria",
    "Belgium",
    "Bosnia and Herzegovina",
    "Botswana", // 60-69
    "Bulgaria",
    "Croatia",
    "Cyprus",
    "Czech Republic",
    "Denmark",
    "Estonia",
    "Finland",
    "France",
    "Germany",
    "Greece", // 70-79
    "Hungary",
    "Iceland",
    "Ireland",
    "Italy",
    "Latvia",
    "Lesotho",
    "Liechtenstein",
    "Lithuania",
    "Luxembourg",
    "Macedonia", // 80-89
    "Malta",
    "Montenegro",
    "Mozambique",
    "Namibia",
    "Netherlands",
    "New Zealand",
    "Norway",
    "Poland",
    "Portugal",
    "Romania", // 90-99
    "Russia",
    "Serbia",
    "Slovakia",
    "Slovenia",
    "South Africa",
    "Spain",
    "Swaziland",
    "Sweden",
    "Switzerland",
    "Turkey", // 100-109
    "United Kingdom",
    "Zambia",
    "Zimbabwe",
    "Azerbaijan",
    "Mauritania",
    "Mali",
    "Niger",
    "Chad",
    "Sudan",
    "Eritrea", // 110-119
    "Djibouti",
    "Somalia",
    "Andorra",
    "Gibraltar",
    "Guernsey",
    "Isle of Man",
    "Jersey",
    "Monaco",
    "Taiwan",
    "", // 120-129
    "",
    "",
    "",
    "",
    "",
    "",
    "South Korea",
    "",
    "",
    "", // 130-139
    "",
    "",
    "",
    "",
    "Hong Kong",
    "Macau",
    "",
    "",
    "",
    "", // 140-149
    "",
    "",
    "Indonesia",
    "Singapore",
    "Thailand",
    "Philippines",
    "Malaysia",
    "",
    "",
    "", // 150-159
    "China",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "United Arab Emirates",
    "India", // 160-169
    "Egypt",
    "Oman",
    "Qatar",
    "Kuwait",
    "Saudi Arabia",
    "Syria",
    "Bahrain",
    "Jordan",
    "",
    "", // 170-179
    "",
    "",
    "",
    "",
    "San Marino",
    "Vatican City",
    "Bermuda", // 180-186
}};

ConfigureSystem::ConfigureSystem(QWidget* parent)
    : QWidget{parent}, ui{std::make_unique<Ui::ConfigureSystem>()} {
    ui->setupUi(this);
    connect(ui->combo_birthmonth,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ConfigureSystem::updateBirthdayComboBox);
    connect(ui->combo_init_clock,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ConfigureSystem::updateInitTime);
    connect(ui->button_regenerate_console_id, &QPushButton::clicked, this,
            &ConfigureSystem::refreshConsoleID);
    for (u8 i{}; i < country_names.size(); i++) {
        if (country_names.at(i) != "") {
            ui->combo_country->addItem(country_names.at(i), i);
        }
    }

    ConfigureTime();
}

ConfigureSystem::~ConfigureSystem() {}

void ConfigureSystem::setConfiguration() {
    enabled = !Core::System::GetInstance().IsPoweredOn();

    ui->combo_init_clock->setCurrentIndex(static_cast<u8>(Settings::values.init_clock));
    QDateTime date_time;
    date_time.setTime_t(Settings::values.init_time);
    ui->edit_init_time->setDateTime(date_time);

    if (!enabled) {
        cfg = Core::System::GetInstance()
                  .ServiceManager()
                  .GetService<Service::CFG::Module::Interface>("cfg:u")
                  ->GetModule();
        ReadSystemSettings();
        ui->group_system_settings->setEnabled(false);
    } else {
        // This tab is enabled only when game is not running (i.e. all services are not
        // initialized).
        cfg = std::make_shared<Service::CFG::Module>();

        ReadSystemSettings();
        ui->label_disable_info->hide();
    }
}

void ConfigureSystem::ReadSystemSettings() {
    // set username
    username = cfg->GetUsername();
    ui->edit_username->setText(QString::fromStdU16String(username));

    // set birthday
    auto [birthmonth, birthday]{cfg->GetBirthday()};
    ui->combo_birthmonth->setCurrentIndex(birthmonth - 1);
    updateBirthdayComboBox(
        birthmonth -
        1); // explicitly update it because the signal from setCurrentIndex is not reliable
    ui->combo_birthday->setCurrentIndex(birthday - 1);

    // set system language
    language_index = cfg->GetSystemLanguage();
    ui->combo_language->setCurrentIndex(language_index);

    // set model
    model_index = static_cast<int>(cfg->GetSystemModel());
    ui->combo_model->setCurrentIndex(model_index);

    // set sound output mode
    sound_index = cfg->GetSoundOutputMode();
    ui->combo_sound->setCurrentIndex(sound_index);

    // set the country code
    country_code = cfg->GetCountryCode();
    ui->combo_country->setCurrentIndex(ui->combo_country->findData(country_code));

    // set the console id
    u64 console_id{cfg->GetConsoleUniqueId()};
    ui->label_console_id->setText(
        QString("Console ID: 0x%1").arg(QString::number(console_id, 16).toUpper()));

    // set the region
    // The first item is "auto-select" with actual value -1, so plus one here will do the trick
    ui->region_combobox->setCurrentIndex(Settings::values.region_value + 1);
}

void ConfigureSystem::applyConfiguration() {
    if (!enabled)
        return;

    bool modified{};

    // apply username
    std::u16string new_username{ui->edit_username->text().toStdU16String()};
    if (new_username != username) {
        cfg->SetUsername(new_username);
        modified = true;
    }

    // apply birthday
    int new_birthmonth{ui->combo_birthmonth->currentIndex() + 1};
    int new_birthday{ui->combo_birthday->currentIndex() + 1};
    if (birthmonth != new_birthmonth || birthday != new_birthday) {
        cfg->SetBirthday(new_birthmonth, new_birthday);
        modified = true;
    }

    // apply language
    int new_language{ui->combo_language->currentIndex()};
    if (language_index != new_language) {
        cfg->SetSystemLanguage(static_cast<Service::CFG::SystemLanguage>(new_language));
        modified = true;
    }

    // apply sound
    int new_sound{ui->combo_sound->currentIndex()};
    if (sound_index != new_sound) {
        cfg->SetSoundOutputMode(static_cast<Service::CFG::SoundOutputMode>(new_sound));
        modified = true;
    }

    // apply model
    int new_model{ui->combo_model->currentIndex()};
    if (model_index != new_model) {
        cfg->SetSystemModel(static_cast<Service::CFG::SystemModel>(new_model));
        modified = true;
    }

    // apply country
    u8 new_country{static_cast<u8>(ui->combo_country->currentData().toInt())};
    if (country_code != new_country) {
        cfg->SetCountryCode(new_country);
        modified = true;
    }

    // update the config savegame if any item except region is modified.
    if (modified)
        cfg->UpdateConfigNANDSavegame();

    Settings::values.init_clock =
        static_cast<Settings::InitClock>(ui->combo_init_clock->currentIndex());
    Settings::values.init_time = ui->edit_init_time->dateTime().toTime_t();
    Settings::values.region_value = ui->region_combobox->currentIndex() - 1;
    Settings::Apply();
}

void ConfigureSystem::updateBirthdayComboBox(int birthmonth_index) {
    if (birthmonth_index < 0 || birthmonth_index >= 12)
        return;

    // store current day selection
    int birthday_index{ui->combo_birthday->currentIndex()};

    // get number of days in the new selected month
    int days{days_in_month[birthmonth_index]};

    // if the selected day is out of range,
    // reset it to 1st
    if (birthday_index < 0 || birthday_index >= days)
        birthday_index = 0;

    // update the day combo box
    ui->combo_birthday->clear();
    for (int i{1}; i <= days; ++i) {
        ui->combo_birthday->addItem(QString::number(i));
    }

    // restore the day selection
    ui->combo_birthday->setCurrentIndex(birthday_index);
}

void ConfigureSystem::ConfigureTime() {
    ui->edit_init_time->setCalendarPopup(true);
    QDateTime dt{};
    dt.fromString("2000-01-01 00:00:01", "yyyy-MM-dd hh:mm:ss");
    ui->edit_init_time->setMinimumDateTime(dt);

    setConfiguration();

    updateInitTime(ui->combo_init_clock->currentIndex());
}

void ConfigureSystem::updateInitTime(int init_clock) {
    const bool is_fixed_time =
        static_cast<Settings::InitClock>(init_clock) == Settings::InitClock::FixedTime;
    ui->label_init_time->setVisible(is_fixed_time);
    ui->edit_init_time->setVisible(is_fixed_time);
}

void ConfigureSystem::refreshConsoleID() {
    QMessageBox::StandardButton reply;
    QString warning_text = "This will replace your current virtual 3DS with a new one."
                           "Your current virtual 3DS will not be recoverable. "
                           "This might have unexpected effects in games. This might fail, "
                           "if you use an outdated config savegame. Continue?";
    reply =
        QMessageBox::critical(this, "Warning", warning_text, QMessageBox::No | QMessageBox::Yes);
    if (reply == QMessageBox::No)
        return;
    u32 random_number;
    u64 console_id;
    cfg->GenerateConsoleUniqueId(random_number, console_id);
    cfg->SetConsoleUniqueId(random_number, console_id);
    cfg->UpdateConfigNANDSavegame();
    ui->label_console_id->setText(
        QString("Console ID: 0x%1").arg(QString::number(console_id, 16).toUpper()));
}
