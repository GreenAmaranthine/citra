// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "video_core/geometry_pipeline.h"
#include "video_core/pica.h"
#include "video_core/pica_state.h"
#include "video_core/renderer/renderer.h"
#include "video_core/video_core.h"

namespace Pica {

State g_state;

void Init() {
    g_state.Reset();
}

void Shutdown() {
    Shader::Shutdown();
}

template <typename T>
void Zero(T& o) {
    std::memset(&o, 0, sizeof(o));
}

State::State() : geometry_pipeline{*this} {
    auto SubmitVertex{[this](const Shader::AttributeBuffer& vertex) {
        primitive_assembler.SubmitVertex(
            Shader::OutputVertex::FromAttributeBuffer(regs.rasterizer, vertex));
    }};

    auto SetWinding{[this]() { primitive_assembler.SetWinding(); }};

    g_state.gs_unit.SetVertexHandler(SubmitVertex, SetWinding);
    g_state.geometry_pipeline.SetVertexHandler(SubmitVertex);
}

void State::Reset() {
    Zero(regs);
    Zero(vs);
    Zero(gs);
    Zero(cmd_list);
    Zero(immediate);
    primitive_assembler.Reconfigure(PipelineRegs::TriangleTopology::List);
}
} // namespace Pica
