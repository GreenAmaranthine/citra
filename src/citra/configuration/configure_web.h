// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QFutureWatcher>
#include <QWidget>

namespace Ui {
class ConfigureWeb;
} // namespace Ui

class ConfigureWeb : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureWeb(QWidget* parent = nullptr);
    ~ConfigureWeb();

    void ApplyConfiguration();

public slots:
    void OnLoginChanged();
    void VerifyLogin();
    void OnLoginVerified();

private:
    void LoadConfiguration();

    bool user_verified{true};
    QFutureWatcher<bool> verify_watcher;

    std::unique_ptr<Ui::ConfigureWeb> ui;
};
