// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <memory>
#include <dynarmic/A32/a32.h>
#include <dynarmic/A32/context.h>
#include "common/common_types.h"
#include "core/cpu/common.h"

namespace Memory {
struct PageTable;
} // namespace Memory

class UserCallbacks;

class ThreadContext final : NonCopyable {
public:
    ThreadContext() {
        Reset();
    }
    ~ThreadContext() = default;

    void Reset() {
        ctx.Regs() = {};
        ctx.SetCpsr(0);
        ctx.ExtRegs() = {};
        ctx.SetFpscr(0);
        fpexc = 0;
    }

    u32 GetCpuRegister(size_t index) const {
        return ctx.Regs()[index];
    }

    void SetCpuRegister(size_t index, u32 value) {
        ctx.Regs()[index] = value;
    }

    void SetProgramCounter(u32 value) {
        return SetCpuRegister(15, value);
    }

    void SetStackPointer(u32 value) {
        return SetCpuRegister(13, value);
    }

    u32 GetCpsr() const {
        return ctx.Cpsr();
    }

    void SetCpsr(u32 value) {
        ctx.SetCpsr(value);
    }

    u32 GetFpuRegister(size_t index) const {
        return ctx.ExtRegs()[index];
    }

    void SetFpuRegister(size_t index, u32 value) {
        ctx.ExtRegs()[index] = value;
    }

    u32 GetFpscr() const {
        return ctx.Fpscr();
    }

    void SetFpscr(u32 value) {
        ctx.SetFpscr(value);
    }

    u32 GetFpexc() const {
        return fpexc;
    }

    void SetFpexc(u32 value) {
        fpexc = value;
    }

private:
    friend class CPU;

    Dynarmic::A32::Context ctx;
    u32 fpexc;
};

class CPU {
public:
    explicit CPU();
    ~CPU();

    void Run();

    void SetPC(u32 pc);
    u32 GetPC() const;
    u32 GetReg(int index) const;
    void SetReg(int index, u32 value);
    u32 GetVFPReg(int index) const;
    void SetVFPReg(int index, u32 value);
    u32 GetVFPSystemReg(VFPSystemRegister reg) const;
    void SetVFPSystemReg(VFPSystemRegister reg, u32 value);
    u32 GetCPSR() const;
    void SetCPSR(u32 cpsr);
    u32 GetCP15Register(CP15Register reg);
    void SetCP15Register(CP15Register reg, u32 value);

    std::unique_ptr<ThreadContext> NewContext() const;
    void SaveContext(const std::unique_ptr<ThreadContext>& arg);
    void LoadContext(const std::unique_ptr<ThreadContext>& arg);

    void PrepareReschedule();

    void ClearInstructionCache();
    void InvalidateCacheRange(u32 start_address, size_t length);
    void PageTableChanged();

    void SyncSettings();

private:
    friend class UserCallbacks;
    std::unique_ptr<UserCallbacks> cb;
    std::unique_ptr<Dynarmic::A32::Jit> MakeJit();

    Dynarmic::A32::Jit* jit{};
    Memory::PageTable* current_page_table{};
    std::map<Memory::PageTable*, std::unique_ptr<Dynarmic::A32::Jit>> jits;
    std::shared_ptr<State> state;
};
