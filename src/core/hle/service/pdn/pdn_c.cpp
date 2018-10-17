// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/pdn/pdn_c.h"

namespace Service::PDN {

PDN_C::PDN_C() : ServiceFramework{"pdn:c"} {}
PDN_C::~PDN_C() = default;

} // namespace Service::PDN
