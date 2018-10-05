// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef _WIN32
#include <windows.h>

#include <wincon.h>
#endif

#include <array>
#include <cmath>
#include "citra/ui_settings.h"
#include "citra/util/util.h"
#include "common/logging/backend.h"

QString ReadableByteSize(qulonglong size) {
    static const std::array<const char*, 6> units{{"B", "KiB", "MiB", "GiB", "TiB", "PiB"}};
    if (size == 0)
        return "0";
    int digit_groups{std::min<int>(static_cast<int>(std::log10(size) / std::log10(1024)),
                                   static_cast<int>(units.size()))};
    return QString("%L1 %2")
        .arg(size / std::pow(1024, digit_groups), 0, 'f', 1)
        .arg(units[digit_groups]);
}

void ToggleConsole() {
    static bool console_shown{};
    if (console_shown == UISettings::values.show_console)
        return;
    else
        console_shown = UISettings::values.show_console;
#ifdef _WIN32
    FILE* temp{};
    if (UISettings::values.show_console)
        if (AllocConsole()) {
            // The first parameter for freopen_s is a out parameter, so we can just ignore it
            freopen_s(&temp, "CONIN$", "r", stdin);
            freopen_s(&temp, "CONOUT$", "w", stdout);
            freopen_s(&temp, "CONOUT$", "w", stderr);
            Log::AddBackend(std::make_unique<Log::ColorConsoleBackend>());
        } else if (FreeConsole()) {
            // In order to close the console, we have to also detach the streams on it.
            // Just redirect them to NUL if there is no console window
            Log::RemoveBackend(Log::ColorConsoleBackend::Name());
            freopen_s(&temp, "NUL", "r", stdin);
            freopen_s(&temp, "NUL", "w", stdout);
            freopen_s(&temp, "NUL", "w", stderr);
        }
#else
    if (UISettings::values.show_console)
        Log::AddBackend(std::make_unique<Log::ColorConsoleBackend>());
    else
        Log::RemoveBackend(Log::ColorConsoleBackend::Name);
#endif
}
