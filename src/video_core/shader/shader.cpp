// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include <cstring>
#include "common/bit_set.h"
#include "common/logging/log.h"
#include "video_core/pica_state.h"
#include "video_core/regs_rasterizer.h"
#include "video_core/regs_shader.h"
#include "video_core/shader/shader.h"
#include "video_core/shader/shader_jit_x64.h"
#include "video_core/video_core.h"

namespace Pica::Shader {

void OutputVertex::ValidateSemantics(const RasterizerRegs& regs) {
    unsigned int num_attributes{regs.vs_output_total};
    ASSERT(num_attributes <= 7);
    for (std::size_t attrib{}; attrib < num_attributes; ++attrib) {
        u32 output_register_map{regs.vs_output_attributes[attrib].raw};
        for (std::size_t comp{}; comp < 4; ++comp) {
            u32 semantic{(output_register_map >> (8 * comp)) & 0x1F};
            ASSERT_MSG(semantic < 24 || semantic == RasterizerRegs::VSOutputAttributes::INVALID,
                       "Invalid/unknown semantic id: {}", semantic);
        }
    }
}

OutputVertex OutputVertex::FromAttributeBuffer(const RasterizerRegs& regs,
                                               const AttributeBuffer& input) {
    // Setup output data
    union {
        OutputVertex ret{};
        // Allow us to overflow OutputVertex to avoid branches, since
        // RasterizerRegs::VSOutputAttributes::INVALID would write to slot 31, which
        // would be out of bounds otherwise.
        std::array<float24, 32> vertex_slots_overflow;
    };

    // Assert that OutputVertex has enough space for 24 semantic registers
    static_assert(sizeof(std::array<float24, 24>) == sizeof(ret),
                  "Struct and array have different sizes.");

    unsigned int num_attributes{regs.vs_output_total & 7};
    for (std::size_t attrib{}; attrib < num_attributes; ++attrib) {
        const auto output_register_map{regs.vs_output_attributes[attrib]};
        vertex_slots_overflow[output_register_map.map_x] = input.attr[attrib][0];
        vertex_slots_overflow[output_register_map.map_y] = input.attr[attrib][1];
        vertex_slots_overflow[output_register_map.map_z] = input.attr[attrib][2];
        vertex_slots_overflow[output_register_map.map_w] = input.attr[attrib][3];
    }

    // The hardware takes the absolute and saturates vertex colors like this, *before* doing
    // interpolation
    for (unsigned i{}; i < 4; ++i) {
        float c{std::fabs(ret.color[i].ToFloat32())};
        ret.color[i] = float24::FromFloat32(c < 1.0f ? c : 1.0f);
    }

    return ret;
}

void UnitState::LoadInput(const ShaderRegs& config, const AttributeBuffer& input) {
    const unsigned max_attribute = config.max_input_attribute_index;

    for (unsigned attr{}; attr <= max_attribute; ++attr) {
        unsigned reg = config.GetRegisterForAttribute(attr);
        registers.input[reg] = input.attr[attr];
    }
}

static void CopyRegistersToOutput(const Math::Vec4<float24>* regs, u32 mask,
                                  AttributeBuffer& buffer) {
    int output_i{};
    for (int reg : BitSet32(mask)) {
        buffer.attr[output_i++] = regs[reg];
    }
}

void UnitState::WriteOutput(const ShaderRegs& config, AttributeBuffer& output) {
    CopyRegistersToOutput(registers.output, config.output_mask, output);
}

UnitState::UnitState(GSEmitter* emitter) : emitter_ptr(emitter) {}

GSEmitter::GSEmitter() {
    handlers = new Handlers;
}

GSEmitter::~GSEmitter() {
    delete handlers;
}

void GSEmitter::Emit(Math::Vec4<float24> (&output_regs)[16]) {
    ASSERT(vertex_id < 3);
    // TODO: This should be merged with UnitState::WriteOutput somehow
    CopyRegistersToOutput(output_regs, output_mask, buffer[vertex_id]);

    if (prim_emit) {
        if (winding)
            handlers->winding_setter();
        for (std::size_t i{}; i < buffer.size(); ++i) {
            handlers->vertex_handler(buffer[i]);
        }
    }
}

GSUnitState::GSUnitState() : UnitState{&emitter} {}

void GSUnitState::SetVertexHandler(VertexHandler vertex_handler, WindingSetter winding_setter) {
    emitter.handlers->vertex_handler = std::move(vertex_handler);
    emitter.handlers->winding_setter = std::move(winding_setter);
}

void GSUnitState::ConfigOutput(const ShaderRegs& config) {
    emitter.output_mask = config.output_mask;
}

static std::unique_ptr<JitX64Engine> jit_engine;

ShaderEngine* GetEngine() {
    if (jit_engine == nullptr) {
        jit_engine = std::make_unique<JitX64Engine>();
    }
    return jit_engine.get();
}

void Shutdown() {
    jit_engine = nullptr;
}

} // namespace Pica::Shader
