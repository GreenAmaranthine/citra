// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/cpu/cpu_cp15.h"

using Callback = Dynarmic::A32::Coprocessor::Callback;
using CallbackOrAccessOneWord = Dynarmic::A32::Coprocessor::CallbackOrAccessOneWord;
using CallbackOrAccessTwoWords = Dynarmic::A32::Coprocessor::CallbackOrAccessTwoWords;

CPUCP15::CPUCP15(State& state) : state{state} {}

CPUCP15::~CPUCP15() = default;

std::optional<Callback> CPUCP15::CompileInternalOperation(bool two, unsigned opc1, CoprocReg CRd,
                                                          CoprocReg CRn, CoprocReg CRm,
                                                          unsigned opc2) {
    return {};
}

CallbackOrAccessOneWord CPUCP15::CompileSendOneWord(bool two, unsigned opc1, CoprocReg CRn,
                                                    CoprocReg CRm, unsigned opc2) {
    // TODO: Privileged CP15 registers
    if (!two && CRn == CoprocReg::C7 && opc1 == 0 && CRm == CoprocReg::C5 && opc2 == 4)
        // This is a dummy write, we ignore the value written here.
        return &state.cp15[CP15_FLUSH_PREFETCH_BUFFER];
    if (!two && CRn == CoprocReg::C7 && opc1 == 0 && CRm == CoprocReg::C10) {
        switch (opc2) {
        case 4:
            // This is a dummy write, we ignore the value written here.
            return &state.cp15[CP15_DATA_SYNC_BARRIER];
        case 5:
            // This is a dummy write, we ignore the value written here.
            return &state.cp15[CP15_DATA_MEMORY_BARRIER];
        default:
            return boost::blank{};
        }
    }
    if (!two && CRn == CoprocReg::C13 && opc1 == 0 && CRm == CoprocReg::C0 && opc2 == 2)
        return &state.cp15[CP15_THREAD_UPRW];
    return {};
}

CallbackOrAccessTwoWords CPUCP15::CompileSendTwoWords(bool two, unsigned opc, CoprocReg CRm) {
    return {};
}

CallbackOrAccessOneWord CPUCP15::CompileGetOneWord(bool two, unsigned opc1, CoprocReg CRn,
                                                   CoprocReg CRm, unsigned opc2) {
    // TODO: Privileged CP15 registers
    if (!two && CRn == CoprocReg::C13 && opc1 == 0 && CRm == CoprocReg::C0) {
        switch (opc2) {
        case 2:
            return &state.cp15[CP15_THREAD_UPRW];
        case 3:
            return &state.cp15[CP15_THREAD_URO];
        default:
            return {};
        }
    }
    return {};
}

CallbackOrAccessTwoWords CPUCP15::CompileGetTwoWords(bool two, unsigned opc, CoprocReg CRm) {
    return {};
}

std::optional<Callback> CPUCP15::CompileLoadWords(bool two, bool long_transfer, CoprocReg CRd,
                                                  std::optional<u8> option) {
    return {};
}

std::optional<Callback> CPUCP15::CompileStoreWords(bool two, bool long_transfer, CoprocReg CRd,
                                                   std::optional<u8> option) {
    return {};
}
