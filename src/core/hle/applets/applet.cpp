// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstddef>
#include <memory>
#include "common/assert.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/hle/applets/applet.h"
#include "core/hle/result.h"

namespace HLE::Applets {

ResultCode Applet::Start(const Service::APT::AppletStartupParameter& parameter) {
    ResultCode result{StartImpl(parameter)};
    if (result.IsError())
        return result;
    // Schedule the update event
    manager.ScheduleEvent(id);
    return result;
}

bool Applet::IsRunning() const {
    return is_running;
}

void Applet::SendParameter(const Service::APT::MessageParameter& parameter) {
    manager.CancelAndSendParameter(parameter);
}

} // namespace HLE::Applets
