// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include "common/common_types.h"

namespace AudioCore {

class Sink {
public:
    explicit Sink(std::string device_id);
    ~Sink();

    /**
     * Set callback for samples
     * @param samples Samples in interleaved stereo PCM16 format.
     * @param sample_count Number of samples.
     */
    void SetCallback(std::function<void(s16*, std::size_t)> cb);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

std::vector<std::string> ListDevices();

} // namespace AudioCore
