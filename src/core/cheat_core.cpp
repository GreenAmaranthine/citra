// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <fmt/format.h>
#include "common/common_paths.h"
#include "common/file_util.h"
#include "core/cheat_core.h"
#include "core/core.h"
#include "core/hle/kernel/process.h"
#include "core/hle/service/hid/hid.h"
#include "core/memory.h"

namespace CheatCore {

static CoreTiming::EventType* tick_event;
static std::unique_ptr<Engine> cheat_engine;

static void CheatTickCallback(u64, int cycles_late) {
    if (!cheat_engine)
        cheat_engine = std::make_unique<Engine>();
    cheat_engine->Run();
    CoreTiming::ScheduleEvent(BASE_CLOCK_RATE_ARM11 - cycles_late, tick_event);
}

void Init() {
    std::string cheats_dir{FileUtil::GetUserPath(FileUtil::UserPath::UserDir) + "cheats"};
    if (!FileUtil::Exists(cheats_dir)) {
        FileUtil::CreateDir(cheats_dir);
    }
    tick_event = CoreTiming::RegisterEvent("CheatCore::tick_event", CheatTickCallback);
    CoreTiming::ScheduleEvent(BASE_CLOCK_RATE_ARM11, tick_event);
}

void Shutdown() {
    CoreTiming::UnscheduleEvent(tick_event, 0);
}

void RefreshCheats() {
    if (!cheat_engine)
        cheat_engine = std::make_unique<Engine>();
    cheat_engine->RefreshCheats();
}

static std::string GetFilePath() {
    return fmt::format("{}cheats/{:016X}.txt", FileUtil::GetUserPath(FileUtil::UserPath::UserDir),
                       Kernel::g_current_process->codeset->program_id);
}

Engine::Engine() {
    const auto file_path{GetFilePath()};
    if (!FileUtil::Exists(file_path))
        FileUtil::CreateEmptyFile(file_path);
    cheats_list = ReadFileContents();
}

std::vector<Cheat> Engine::ReadFileContents() {
    std::string file_path{GetFilePath()};
    std::string contents;
    FileUtil::ReadFileToString(true, file_path.c_str(), contents);

    std::vector<std::string> lines;
    Common::SplitString(contents, '\n', lines);

    std::vector<CheatLine> cheat_lines;
    std::vector<Cheat> cheats;
    std::string name;
    bool enabled{};
    for (std::size_t i{}; i < lines.size(); i++) {
        std::string current_line{lines[i]};
        current_line = Common::Trim(current_line);
        if (!current_line.empty()) {
            if (current_line.compare(0, 2, "+[") == 0) { // Enabled code
                if (!cheat_lines.empty()) {
                    cheats.push_back(Cheat(name, cheat_lines, enabled));
                }
                name = current_line.substr(2, current_line.length() - 3);
                cheat_lines.clear();
                enabled = true;
                continue;
            } else if (current_line.front() == '[') { // Disabled code
                if (!cheat_lines.empty()) {
                    cheats.push_back(Cheat(name, cheat_lines, enabled));
                }
                name = current_line.substr(1, current_line.length() - 2);
                cheat_lines.clear();
                enabled = false;
                continue;
            } else if (current_line.front() != '*') {
                cheat_lines.emplace_back(std::move(current_line));
            }
        }
        if (i == lines.size() - 1) { // End of file
            if (!cheat_lines.empty()) {
                cheats.push_back(Cheat(name, cheat_lines, enabled));
            }
        }
    }
    return cheats;
}

void Engine::Save(std::vector<Cheat> cheats) {
    std::string file_path{GetFilePath()};
    FileUtil::IOFile file{file_path, "w+"};
    for (auto& cheat : cheats) {
        std::string str{cheat.ToString()};
        file.WriteBytes(str.c_str(), str.length());
    }
}

void Engine::RefreshCheats() {
    std::string file_path{GetFilePath()};
    if (!FileUtil::Exists(file_path))
        FileUtil::CreateEmptyFile(file_path);
    cheats_list = ReadFileContents();
}

void Engine::Run() {
    for (auto& cheat : cheats_list) {
        cheat.Execute();
    }
}

void Cheat::Execute() {
    if (!enabled)
        return;
    u32 addr{};
    u32 reg{};
    u32 offset{};
    u32 val{};
    int if_flag{};
    u32 loop_count{};
    std::size_t loopbackline{};
    bool loop_flag{};
    for (std::size_t i{}; i < cheat_lines.size(); i++) {
        CheatLine line{cheat_lines[i]};
        if (line.type == CheatType::Null)
            continue;
        addr = line.address;
        val = line.value;
        if (if_flag > 0) {
            if (line.type == CheatType::Patch)
                i += (line.value + 7) / 8;
            if (line.type == CheatType::Terminator)
                if_flag--; // ENDIF
            if (line.type == CheatType::FullTerminator) {
                // NEXT & Flush
                if (loop_flag)
                    i = loopbackline - 1;
                else {
                    offset = 0;
                    reg = 0;
                    loop_count = 0;
                    if_flag = 0;
                    loop_flag = false;
                }
            }
            continue;
        }

        switch (line.type) {
        case CheatType::Write32: { // 0XXXXXXX YYYYYYYY   word[XXXXXXX+offset] = YYYYYYYY
            addr = line.address + offset;
            Memory::Write32(addr, val);
            Core::CPU().InvalidateCacheRange(addr, sizeof(u32));
            break;
        }
        case CheatType::Write16: { // 1XXXXXXX 0000YYYY   half[XXXXXXX+offset] = YYYY
            addr = line.address + offset;
            Memory::Write16(addr, static_cast<u16>(val));
            Core::CPU().InvalidateCacheRange(addr, sizeof(u16));
            break;
        }
        case CheatType::Write8: { // 2XXXXXXX 000000YY   byte[XXXXXXX+offset] = YY
            addr = line.address + offset;
            Memory::Write8(addr, static_cast<u8>(val));
            Core::CPU().InvalidateCacheRange(addr, sizeof(u8));
            break;
        }
        case CheatType::GreaterThan32: { // 3XXXXXXX YYYYYYYY   IF YYYYYYYY > word[XXXXXXX]
                                         // ;unsigned
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value > val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::LessThan32: { // 4XXXXXXX YYYYYYYY   IF YYYYYYYY < word[XXXXXXX]   ;unsigned
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value < val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::EqualTo32: { // 5XXXXXXX YYYYYYYY   IF YYYYYYYY = word[XXXXXXX]
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value == val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::NotEqualTo32: { // 6XXXXXXX YYYYYYYY   IF YYYYYYYY <> word[XXXXXXX]
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value != val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::GreaterThan16: { // 7XXXXXXX ZZZZYYYY   IF YYYY > ((not ZZZZ) AND
                                         // half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            u32 x{line.address & 0x0FFFFFFF};
            u32 y{line.value & 0xFFFF};
            u32 z{line.value >> 16};
            val = Memory::Read16(x);
            if (y > (u16)((~z) & val)) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::LessThan16: { // 8XXXXXXX ZZZZYYYY   IF YYYY < ((not ZZZZ) AND
                                      // half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            u32 x{line.address & 0x0FFFFFFF};
            u32 y{line.value & 0xFFFF};
            u32 z{line.value >> 16};
            val = Memory::Read16(x);
            if (y < (u16)((~z) & val)) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::EqualTo16: { // 9XXXXXXX ZZZZYYYY   IF YYYY = ((not ZZZZ) AND half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            u32 x{line.address & 0x0FFFFFFF};
            u32 y{line.value & 0xFFFF};
            u32 z{line.value >> 16};
            val = Memory::Read16(x);
            if (y == (u16)((~z) & val)) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::NotEqualTo16: { // AXXXXXXX ZZZZYYYY   IF YYYY <> ((not ZZZZ) AND
                                        // half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            u32 x{line.address & 0x0FFFFFFF};
            u32 y{line.value & 0xFFFF};
            u32 z{line.value >> 16};
            val = Memory::Read16(x);
            if (y != (u16)((~z) & val)) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::LoadOffset: { // BXXXXXXX 00000000   offset = word[XXXXXXX+offset]
            addr = line.address + offset;
            offset = Memory::Read32(addr);
            break;
        }
        case CheatType::Loop: {
            if (loop_count < (line.value + 1))
                loop_flag = true;
            else
                loop_flag = false;
            loop_count++;
            loopbackline = i;
            break;
        }
        case CheatType::Terminator: {
            break;
        }
        case CheatType::LoopExecuteVariant: {
            if (loop_flag)
                i = loopbackline - 1;
            break;
        }
        case CheatType::FullTerminator: {
            if (loop_flag)
                i = loopbackline - 1;
            else {
                offset = 0;
                reg = 0;
                loop_count = 0;
                if_flag = 0;
                loop_flag = false;
            }
            break;
        }
        case CheatType::SetOffset: {
            offset = line.value;
            break;
        }
        case CheatType::AddValue: {
            reg += line.value;
            break;
        }
        case CheatType::SetValue: {
            reg = line.value;
            break;
        }
        case CheatType::IncrementiveWrite32: {
            addr = line.value + offset;
            Memory::Write32(addr, reg);
            offset += 4;
            break;
        }
        case CheatType::IncrementiveWrite16: {
            addr = line.value + offset;
            Memory::Write16(addr, static_cast<u16>(reg));
            offset += 2;
            break;
        }
        case CheatType::IncrementiveWrite8: {
            addr = line.value + offset;
            Memory::Write8(addr, static_cast<u8>(reg));
            offset += 1;
            break;
        }
        case CheatType::Load32: {
            addr = line.value + offset;
            reg = Memory::Read32(addr);
            break;
        }
        case CheatType::Load16: {
            addr = line.value + offset;
            reg = Memory::Read16(addr);
            break;
        }
        case CheatType::Load8: {
            addr = line.value + offset;
            reg = Memory::Read8(addr);
            break;
        }
        case CheatType::AddOffset: {
            offset += line.value;
            break;
        }
        case CheatType::Joker: {
            auto state{Service::HID::GetInputsThisFrame()};
            bool result{(state.hex & line.value) == line.value};
            if (result) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::Patch: {
            // Patch Code (Miscellaneous Memory Manipulation Codes)
            // EXXXXXXX YYYYYYYY
            // Copies YYYYYYYY bytes from (current code location + 8) to [XXXXXXXX + offset].
            u32 x{line.address & 0x0FFFFFFF};
            u32 y{line.value};
            addr = x + offset;
            {
                u32 j{}, t{}, b{};
                if (y > 0)
                    i++; // skip over the current code
                while (y >= 4) {
                    u32 tmp{(t == 0) ? cheat_lines[i].address : cheat_lines[i].value};
                    if (t == 1)
                        i++;
                    t ^= 1;
                    Memory::Write32(addr, tmp);
                    addr += 4;
                    y -= 4;
                }
                while (y > 0) {
                    u32 tmp{((t == 0) ? cheat_lines[i].address : cheat_lines[i].value) >> b};
                    Memory::Write8(addr, tmp);
                    addr += 1;
                    y -= 1;
                    b += 4;
                }
            }
            break;
        }
        }
    }
}

std::string Cheat::ToString() {
    std::string result;
    if (cheat_lines.empty())
        return result;
    if (enabled)
        result += '+';
    result += '[' + name + "]\n";
    for (const auto& line : cheat_lines)
        result += line.cheat_line + '\n';
    result += '\n';
    return result;
}

} // namespace CheatCore
