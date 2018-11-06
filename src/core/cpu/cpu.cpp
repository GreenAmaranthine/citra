// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <dynarmic/A32/a32.h>
#include <dynarmic/A32/context.h>
#include "common/assert.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu/cpu.h"
#include "core/cpu/cpu_cp15.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/lock.h"
#include "core/memory.h"
#include "core/settings.h"

static std::unordered_map<u64, u64> custom_ticks_map{{
    {0x000400000008C300, 570},   {0x000400000008C400, 570},   {0x000400000008C500, 570},
    {0x0004000000126A00, 570},   {0x0004000000126B00, 570},   {0x0004000200120C01, 570},
    {0x000400000F700E00, 18000}, {0x0004000000055D00, 17000}, {0x0004000000055E00, 17000},
    {0x000400000011C400, 17000}, {0x000400000011C500, 17000}, {0x0004000000164800, 17000},
    {0x0004000000175E00, 17000}, {0x00040000001B5000, 17000}, {0x00040000001B5100, 17000},
    {0x00040000001BC500, 27000}, {0x00040000001BC600, 27000}, {0x000400000016E100, 27000},
    {0x0004000000055F00, 27000}, {0x0004000000076500, 27000}, {0x0004000000076400, 27000},
    {0x00040000000D0000, 27000}, {0x0004000000126100, 6000},  {0x0004000000126300, 6000},
    {0x000400000011D700, 6000},
}};

class UserCallbacks final : public Dynarmic::A32::UserCallbacks {
public:
    explicit UserCallbacks(Cpu& parent, Core::System& system) : parent{parent}, system{system} {
        SyncSettings();
    }

    ~UserCallbacks() = default;

    std::uint8_t MemoryRead8(VAddr vaddr) override {
        return Memory::Read8(vaddr);
    }

    std::uint16_t MemoryRead16(VAddr vaddr) override {
        return Memory::Read16(vaddr);
    }

    std::uint32_t MemoryRead32(VAddr vaddr) override {
        return Memory::Read32(vaddr);
    }

    std::uint64_t MemoryRead64(VAddr vaddr) override {
        return Memory::Read64(vaddr);
    }

    void MemoryWrite8(VAddr vaddr, std::uint8_t value) override {
        Memory::Write8(vaddr, value);
    }

    void MemoryWrite16(VAddr vaddr, std::uint16_t value) override {
        Memory::Write16(vaddr, value);
    }

    void MemoryWrite32(VAddr vaddr, std::uint32_t value) override {
        Memory::Write32(vaddr, value);
    }

    void MemoryWrite64(VAddr vaddr, std::uint64_t value) override {
        Memory::Write64(vaddr, value);
    }

    void InterpreterFallback(VAddr pc, std::size_t num_instructions) override {
        ASSERT_MSG(false, "Interpreter fallback (pc={}, num_instructions={})", static_cast<u32>(pc),
                   num_instructions);
    }

    void CallSVC(std::uint32_t swi) override {
        // Lock the global kernel mutex when we enter the kernel HLE.
        std::lock_guard lock{HLE::g_hle_lock};
        switch (swi) {
        case 0x01: { // ControlMemory
            u32 out{};
            parent.SetReg(0, Kernel::SVC::ControlMemory(system, &out, parent.GetReg(0),
                                                        parent.GetReg(1), parent.GetReg(2),
                                                        parent.GetReg(3), parent.GetReg(4))
                                 .raw);
            parent.SetReg(1, out);
            break;
        }
        case 0x02: { // QueryMemory
            Kernel::SVC::MemoryInfo memory_info{};
            Kernel::SVC::PageInfo page_info{};
            parent.SetReg(
                0,
                Kernel::SVC::QueryMemory(system, &memory_info, &page_info, parent.GetReg(2)).raw);
            parent.SetReg(1, memory_info.base_address);
            parent.SetReg(2, memory_info.size);
            parent.SetReg(3, memory_info.permission);
            parent.SetReg(4, memory_info.state);
            parent.SetReg(5, page_info.flags);
            break;
        }
        case 0x03: // ExitProcess
            Kernel::SVC::ExitProcess(system);
            break;
        case 0x08: { // CreateThread
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::CreateThread(system, &handle, parent.GetReg(0),
                                                       parent.GetReg(1), parent.GetReg(2),
                                                       parent.GetReg(3), parent.GetReg(4))
                                 .raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x09: // ExitThread
            Kernel::SVC::ExitThread(system);
            break;
        case 0x0A: // SleepThread
            Kernel::SVC::SleepThread(system, ((s64)parent.GetReg(1) << 32) | parent.GetReg(0));
            break;
        case 0x0B: { // GetThreadPriority
            u32 priority{};
            parent.SetReg(0,
                          Kernel::SVC::GetThreadPriority(system, &priority, parent.GetReg(1)).raw);
            parent.SetReg(1, priority);
            break;
        }
        case 0x0C: // SetThreadPriority
            parent.SetReg(
                0, Kernel::SVC::SetThreadPriority(system, parent.GetReg(0), parent.GetReg(1)).raw);
            break;
        case 0x13: { // CreateMutex
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::CreateMutex(system, &handle, parent.GetReg(1)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x14: // ReleaseMutex
            parent.SetReg(0, Kernel::SVC::ReleaseMutex(system, parent.GetReg(0)).raw);
            break;
        case 0x15: { // CreateSemaphore
            Kernel::Handle handle{};
            parent.SetReg(
                0, Kernel::SVC::CreateSemaphore(system, &handle, parent.GetReg(1), parent.GetReg(2))
                       .raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x16: { // ReleaseSemaphore
            s32 released{};
            parent.SetReg(0, Kernel::SVC::ReleaseSemaphore(system, &released, parent.GetReg(1),
                                                           parent.GetReg(2))
                                 .raw);
            parent.SetReg(1, released);
            break;
        }
        case 0x17: { // CreateEvent
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::CreateEvent(system, &handle, parent.GetReg(1)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x18: // SignalEvent
            parent.SetReg(0, Kernel::SVC::SignalEvent(system, parent.GetReg(0)).raw);
            break;
        case 0x19: // ClearEvent
            parent.SetReg(0, Kernel::SVC::ClearEvent(system, parent.GetReg(0)).raw);
            break;
        case 0x1A: { // CreateTimer
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::CreateTimer(system, &handle, parent.GetReg(1)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x1B: // SetTimer
            parent.SetReg(0, Kernel::SVC::SetTimer(
                                 system, parent.GetReg(0),
                                 static_cast<s64>(((u64)parent.GetReg(3) << 32) | parent.GetReg(2)),
                                 static_cast<s64>(((u64)parent.GetReg(4) << 32) | parent.GetReg(1)))
                                 .raw);
            break;
        case 0x1C: // CancelTimer
            parent.SetReg(0, Kernel::SVC::CancelTimer(system, parent.GetReg(0)).raw);
            break;
        case 0x1D: // ClearTimer
            parent.SetReg(0, Kernel::SVC::ClearTimer(system, parent.GetReg(0)).raw);
            break;
        case 0x1E: { // CreateMemoryBlock
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::CreateMemoryBlock(system, &handle, parent.GetReg(1),
                                                            parent.GetReg(2), parent.GetReg(3),
                                                            parent.GetReg(0))
                                 .raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x1F: // MapMemoryBlock
            parent.SetReg(0, Kernel::SVC::MapMemoryBlock(system, parent.GetReg(0), parent.GetReg(1),
                                                         parent.GetReg(2), parent.GetReg(3))
                                 .raw);
            break;
        case 0x20: // UnmapMemoryBlock
            parent.SetReg(
                0, Kernel::SVC::UnmapMemoryBlock(system, parent.GetReg(0), parent.GetReg(1)).raw);
            break;
        case 0x21: { // CreateAddressArbiter
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::CreateAddressArbiter(system, &handle).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x22: // ArbitrateAddress
            parent.SetReg(
                0, Kernel::SVC::ArbitrateAddress(system, parent.GetReg(0), parent.GetReg(1),
                                                 parent.GetReg(2), parent.GetReg(3),
                                                 (((s64)parent.GetReg(5) << 32) | parent.GetReg(4)))
                       .raw);
            break;
        case 0x23: // CloseHandle
            parent.SetReg(0, Kernel::SVC::CloseHandle(system, parent.GetReg(0)).raw);
            break;
        case 0x24: // WaitSynchronization1
            parent.SetReg(0, Kernel::SVC::WaitSynchronization1(
                                 system, parent.GetReg(0),
                                 static_cast<s64>(((u64)parent.GetReg(3) << 32) | parent.GetReg(2)))
                                 .raw);
            break;
        case 0x25: { // WaitSynchronizationN
            s32 out{};
            parent.SetReg(0, Kernel::SVC::WaitSynchronizationN(
                                 system, &out, parent.GetReg(1), parent.GetReg(2), parent.GetReg(3),
                                 static_cast<s64>(((u64)parent.GetReg(5) << 32) | parent.GetReg(4)))
                                 .raw);
            parent.SetReg(1, out);
            break;
        }
        case 0x27: { // DuplicateHandle
            Kernel::Handle out{};
            parent.SetReg(0, Kernel::SVC::DuplicateHandle(system, &out, parent.GetReg(1)).raw);
            parent.SetReg(1, out);
            break;
        }
        case 0x28: { // GetSystemTick
            s64 res{Kernel::SVC::GetSystemTick(system)};
            parent.SetReg(0, (u32)(res & 0xFFFFFFFF));
            parent.SetReg(1, (u32)((res >> 32) & 0xFFFFFFFF));
            break;
        }
        case 0x2A: { // GetSystemInfo
            s64 out{};
            parent.SetReg(
                0,
                Kernel::SVC::GetSystemInfo(system, &out, parent.GetReg(1), parent.GetReg(2)).raw);
            parent.SetReg(1, (u32)out);
            parent.SetReg(2, (u32)(out >> 32));
            break;
        }
        case 0x2B: { // GetProcessInfo
            s64 out{};
            parent.SetReg(
                0,
                Kernel::SVC::GetProcessInfo(system, &out, parent.GetReg(1), parent.GetReg(2)).raw);
            parent.SetReg(1, (u32)out);
            parent.SetReg(2, (u32)(out >> 32));
            break;
        }
        case 0x2D: { // ConnectToPort
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::ConnectToPort(system, &handle, parent.GetReg(1)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x32: // SendSyncRequest
            parent.SetReg(0, Kernel::SVC::SendSyncRequest(system, parent.GetReg(0)).raw);
            break;
        case 0x33: { // OpenProcess
            Kernel::Handle handle{};
            parent.SetReg(0, Kernel::SVC::OpenProcess(system, &handle, parent.GetReg(1)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x34: { // OpenThread
            Kernel::Handle handle{};
            parent.SetReg(
                0,
                Kernel::SVC::OpenThread(system, &handle, parent.GetReg(1), parent.GetReg(2)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x35: { // GetProcessId
            u32 process_id{};
            parent.SetReg(0, Kernel::SVC::GetProcessId(system, &process_id, parent.GetReg(1)).raw);
            parent.SetReg(1, process_id);
            break;
        }
        case 0x36: { // GetProcessIdOfThread
            u32 process_id{};
            parent.SetReg(
                0, Kernel::SVC::GetProcessIdOfThread(system, &process_id, parent.GetReg(1)).raw);
            parent.SetReg(1, process_id);
            break;
        }
        case 0x37: { // GetThreadId
            u32 thread_id{};
            parent.SetReg(0, Kernel::SVC::GetThreadId(system, &thread_id, parent.GetReg(1)).raw);
            parent.SetReg(1, thread_id);
            break;
        }
        case 0x38: { // GetResourceLimit
            Kernel::Handle resource_limit{};
            parent.SetReg(
                0, Kernel::SVC::GetResourceLimit(system, &resource_limit, parent.GetReg(1)).raw);
            parent.SetReg(1, resource_limit);
            break;
        }
        case 0x39: // GetResourceLimitLimitValues
            parent.SetReg(0, Kernel::SVC::GetResourceLimitLimitValues(
                                 system, parent.GetReg(0), parent.GetReg(1), parent.GetReg(2),
                                 parent.GetReg(3))
                                 .raw);
            break;
        case 0x3A: // GetResourceLimitCurrentValues
            parent.SetReg(0, Kernel::SVC::GetResourceLimitCurrentValues(
                                 system, parent.GetReg(0), parent.GetReg(1), parent.GetReg(2),
                                 parent.GetReg(3))
                                 .raw);
            break;
        case 0x3C: // Break
            Kernel::SVC::Break(system, (u8)parent.GetReg(0));
            break;
        case 0x3D: // OutputDebugString
            Kernel::SVC::OutputDebugString(system, parent.GetReg(0), parent.GetReg(1));
            break;
        case 0x47: { // CreatePort
            Kernel::Handle server_port{}, client_port{};
            parent.SetReg(0, Kernel::SVC::CreatePort(system, &server_port, &client_port,
                                                     parent.GetReg(2), parent.GetReg(3))
                                 .raw);
            parent.SetReg(1, server_port);
            parent.SetReg(1, client_port);
            break;
        }
        case 0x48: { // CreateSessionToPort
            Kernel::Handle handle{};
            parent.SetReg(0,
                          Kernel::SVC::CreateSessionToPort(system, &handle, parent.GetReg(1)).raw);
            parent.SetReg(1, handle);
            break;
        }
        case 0x49: { // CreateSession
            Kernel::Handle server_session{}, client_session{};
            parent.SetReg(0,
                          Kernel::SVC::CreateSession(system, &server_session, &client_session).raw);
            parent.SetReg(1, server_session);
            parent.SetReg(2, server_session);
            break;
        }
        case 0x4A: { // AcceptSession
            Kernel::Handle out{};
            parent.SetReg(0, Kernel::SVC::AcceptSession(system, &out, parent.GetReg(1)).raw);
            parent.SetReg(1, out);
            break;
        }
        case 0x4F: { // ReplyAndReceive
            s32 index{};
            parent.SetReg(0, Kernel::SVC::ReplyAndReceive(system, &index, parent.GetReg(1),
                                                          parent.GetReg(2), parent.GetReg(3))
                                 .raw);
            parent.SetReg(1, index);
            break;
        }
        case 0x70: // ControlProcessMemory
            parent.SetReg(0, Kernel::SVC::ControlProcessMemory(
                                 system, parent.GetReg(0), parent.GetReg(1), parent.GetReg(2),
                                 parent.GetReg(3), parent.GetReg(4), parent.GetReg(5))
                                 .raw);
            break;
        case 0x71: // MapProcessMemory
            parent.SetReg(0, Kernel::SVC::MapProcessMemory(system, parent.GetReg(0),
                                                           parent.GetReg(1), parent.GetReg(2))
                                 .raw);
            break;
        case 0x72: // UnmapProcessMemory
            parent.SetReg(0, Kernel::SVC::UnmapProcessMemory(system, parent.GetReg(0),
                                                             parent.GetReg(1), parent.GetReg(2))
                                 .raw);
            break;
        case 0x7C: // KernelSetState
            parent.SetReg(0, Kernel::SVC::KernelSetState(system, parent.GetReg(0), parent.GetReg(1),
                                                         parent.GetReg(2), parent.GetReg(3))
                                 .raw);
            break;
        case 0x7D: { // QueryProcessMemory
            Kernel::SVC::MemoryInfo memory_info{};
            Kernel::SVC::PageInfo page_info{};
            parent.SetReg(0, Kernel::SVC::QueryProcessMemory(system, &memory_info, &page_info,
                                                             parent.GetReg(2), parent.GetReg(3))
                                 .raw);
            parent.SetReg(1, memory_info.base_address);
            parent.SetReg(2, memory_info.size);
            parent.SetReg(3, memory_info.permission);
            parent.SetReg(4, memory_info.state);
            parent.SetReg(5, page_info.flags);
            break;
        }
        default:
            LOG_ERROR(Kernel_SVC, "Unimplemented SVC 0x{:02X}", swi);
            return;
        }
        LOG_DEBUG(Kernel_SVC, "SVC 0x{:02X} called", swi);
    }

    void ExceptionRaised(VAddr pc, Dynarmic::A32::Exception exception) override {
        ASSERT_MSG(false, "ExceptionRaised(exception={}, pc={:08X}, code={:08X})",
                   static_cast<std::size_t>(exception), pc, MemoryReadCode(pc));
    }

    void AddTicks(std::uint64_t ticks) override {
        system.CoreTiming().AddTicks(use_custom_ticks ? custom_ticks : ticks);
    }

    std::uint64_t GetTicksRemaining() override {
        s64 ticks{system.CoreTiming().GetDowncount()};
        return static_cast<u64>(ticks <= 0 ? 0 : ticks);
    }

    void SyncSettings() {
        custom_ticks = Settings::values.ticks;
        use_custom_ticks = Settings::values.ticks_mode != Settings::TicksMode::Accurate;
        switch (Settings::values.ticks_mode) {
        case Settings::TicksMode::Custom: {
            custom_ticks = Settings::values.ticks;
            use_custom_ticks = true;
            break;
        }
        case Settings::TicksMode::Auto: {
            u64 program_id{};
            system.GetAppLoader().ReadProgramId(program_id);
            auto itr{custom_ticks_map.find(program_id)};
            if (itr != custom_ticks_map.end()) {
                custom_ticks = itr->second;
                use_custom_ticks = true;
            } else {
                custom_ticks = 0;
                use_custom_ticks = false;
            }
            break;
        }
        case Settings::TicksMode::Accurate: {
            custom_ticks = 0;
            use_custom_ticks = false;
            break;
        }
        }
    }

private:
    Cpu& parent;
    u64 custom_ticks{};
    bool use_custom_ticks{};
    Core::System& system;
};

ThreadContext::ThreadContext() {
    ctx = new Dynarmic::A32::Context;
    Reset();
}

ThreadContext::~ThreadContext() = default;

void ThreadContext::Reset() {
    ctx->Regs() = {};
    ctx->SetCpsr(0);
    ctx->ExtRegs() = {};
    ctx->SetFpscr(0);
    fpexc = 0;
}

u32 ThreadContext::GetCpuRegister(std::size_t index) const {
    return ctx->Regs()[index];
}

void ThreadContext::SetCpuRegister(std::size_t index, u32 value) {
    ctx->Regs()[index] = value;
}

void ThreadContext::SetProgramCounter(u32 value) {
    return SetCpuRegister(15, value);
}

void ThreadContext::SetStackPointer(u32 value) {
    return SetCpuRegister(13, value);
}

u32 ThreadContext::GetCpsr() const {
    return ctx->Cpsr();
}

void ThreadContext::SetCpsr(u32 value) {
    ctx->SetCpsr(value);
}

u32 ThreadContext::GetFpuRegister(std::size_t index) const {
    return ctx->ExtRegs()[index];
}

void ThreadContext::SetFpuRegister(std::size_t index, u32 value) {
    ctx->ExtRegs()[index] = value;
}

u32 ThreadContext::GetFpscr() const {
    return ctx->Fpscr();
}

void ThreadContext::SetFpscr(u32 value) {
    ctx->SetFpscr(value);
}

u32 ThreadContext::GetFpexc() const {
    return fpexc;
}

void ThreadContext::SetFpexc(u32 value) {
    fpexc = value;
}

Cpu::Cpu(Core::System& system) : cb{std::make_unique<UserCallbacks>(*this, system)} {
    PageTableChanged();
}

Cpu::~Cpu() = default;

void Cpu::Run() {
    ASSERT(Memory::GetCurrentPageTable() == current_page_table);
    jit->Run();
}

void Cpu::SetPC(u32 pc) {
    jit->Regs()[15] = pc;
}

u32 Cpu::GetPC() const {
    return jit->Regs()[15];
}

u32 Cpu::GetReg(int index) const {
    return jit->Regs()[index];
}

void Cpu::SetReg(int index, u32 value) {
    jit->Regs()[index] = value;
}

u32 Cpu::GetVFPReg(int index) const {
    return jit->ExtRegs()[index];
}

void Cpu::SetVFPReg(int index, u32 value) {
    jit->ExtRegs()[index] = value;
}

u32 Cpu::GetVFPSystemReg(VFPSystemRegister reg) const {
    if (reg == VFP_FPSCR)
        return jit->Fpscr();
    // Dynarmic doesn't implement and/or expose other VFP registers
    return state.vfp[reg];
}

void Cpu::SetVFPSystemReg(VFPSystemRegister reg, u32 value) {
    if (reg == VFP_FPSCR)
        jit->SetFpscr(value);
    // Dynarmic doesn't implement and/or expose other VFP registers
    state.vfp[reg] = value;
}

u32 Cpu::GetCPSR() const {
    return jit->Cpsr();
}

void Cpu::SetCPSR(u32 cpsr) {
    jit->SetCpsr(cpsr);
}

u32 Cpu::GetCP15Register(CP15Register reg) {
    return state.cp15[reg];
}

void Cpu::SetCP15Register(CP15Register reg, u32 value) {
    state.cp15[reg] = value;
}

std::unique_ptr<ThreadContext> Cpu::NewContext() const {
    return std::make_unique<ThreadContext>();
}

void Cpu::SaveContext(const std::unique_ptr<ThreadContext>& arg) {
    auto ctx{dynamic_cast<ThreadContext*>(arg.get())};
    ASSERT(ctx);
    jit->SaveContext(*ctx->ctx);
    ctx->fpexc = state.vfp[VFP_FPEXC];
}

void Cpu::LoadContext(const std::unique_ptr<ThreadContext>& arg) {
    auto ctx{dynamic_cast<ThreadContext*>(arg.get())};
    ASSERT(ctx);
    jit->LoadContext(*ctx->ctx);
    state.vfp[VFP_FPEXC] = ctx->fpexc;
}

void Cpu::PrepareReschedule() {
    if (jit->IsExecuting()) {
        jit->HaltExecution();
    }
}

void Cpu::InvalidateCacheRange(u32 start_address, std::size_t length) {
    jit->InvalidateCacheRange(start_address, length);
}

void Cpu::PageTableChanged() {
    current_page_table = Memory::GetCurrentPageTable();
    auto iter{jits.find(current_page_table)};
    if (iter != jits.end()) {
        jit = iter->second.get();
        return;
    }
    auto new_jit{MakeJit()};
    jit = new_jit.get();
    jits.emplace(current_page_table, std::move(new_jit));
}

void Cpu::SyncSettings() {
    cb->SyncSettings();
}

std::unique_ptr<Dynarmic::A32::Jit> Cpu::MakeJit() {
    Dynarmic::A32::UserConfig config;
    config.callbacks = cb.get();
    config.page_table = &current_page_table->pointers;
    config.coprocessors[15] = std::make_shared<CPUCP15>(state);
    config.define_unpredictable_behaviour = true;
    return std::make_unique<Dynarmic::A32::Jit>(config);
}
