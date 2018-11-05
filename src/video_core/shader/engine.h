// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <unordered_map>
#include "common/common_types.h"

namespace Pica::Shader {

class Shader;
struct ShaderSetup;
struct UnitState;

class ShaderEngine {
public:
    ShaderEngine();
    ~ShaderEngine();

    /**
     * Performs any shader unit setup that only needs to happen once per shader (as opposed to once
     * per vertex, which would happen within the Run function).
     */
    void SetupBatch(ShaderSetup& setup, unsigned int entry_point);

    /**
     * Runs the currently setup shader.
     *
     * @param setup Shader engine state, must be setup with SetupBatch on each shader change.
     * @param state Shader unit state, must be setup with input data before each shader invocation.
     */
    void Run(const ShaderSetup& setup, UnitState& state) const;

private:
    std::unordered_map<u64, std::unique_ptr<Shader>> cache;
};

} // namespace Pica::Shader
