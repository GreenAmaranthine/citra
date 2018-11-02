// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <vector>
#include "common/string_util.h"
#include "core/core_timing.h"

namespace CheatCore {

enum class CheatType {
    Null = -0x1,
    Write32 = 0x00,
    Write16 = 0x01,
    Write8 = 0x02,
    GreaterThan32 = 0x03,
    LessThan32 = 0x04,
    EqualTo32 = 0x05,
    NotEqualTo32 = 0x06,
    GreaterThan16 = 0x07,
    LessThan16 = 0x08,
    EqualTo16 = 0x09,
    NotEqualTo16 = 0x0A,
    LoadOffset = 0x0B,
    Loop = 0x0C,
    Terminator = 0xD0,
    LoopExecuteVariant = 0xD1,
    FullTerminator = 0xD2,
    SetOffset = 0xD3,
    AddValue = 0xD4,
    SetValue = 0xD5,
    IncrementiveWrite32 = 0xD6,
    IncrementiveWrite16 = 0xD7,
    IncrementiveWrite8 = 0xD8,
    Load32 = 0xD9,
    Load16 = 0xDA,
    Load8 = 0xDB,
    AddOffset = 0xDC,
    Joker = 0xDD,
    Patch = 0x0E,
};

/// Represents a single line of a cheat, i.e. 1xxxxxxxx yyyyyyyy
struct CheatLine {
    explicit CheatLine(std::string line) {
        line = std::string(line.c_str()); // remove '/0' characters if any.
        line = Common::Trim(line);
        try {
            std::string type_temp{line.substr(0, 1)};
            // 0xD types have extra subtype value, i.e. 0xDA
            std::string sub_type_temp{};
            if (type_temp == "D" || type_temp == "d")
                sub_type_temp = line.substr(1, 1);
            std::string complete_type_str{type_temp + sub_type_temp};
            type = static_cast<CheatType>(std::stoi(complete_type_str, 0, 16));
            std::vector<std::string> out;
            Common::SplitString(line.substr(complete_type_str.length()), ' ', out);
            if (out.size() == 2) {
                address = std::stoi(out[0], 0, 16);
                value = std::stoi(out[1], 0, 16);
                cheat_line = line;
            } else {
                type = CheatType::Null;
                cheat_line = line;
            }
        } catch (const std::exception&) {
            type = CheatType::Null;
            cheat_line = line;
            return;
        }
    }

    CheatType type;
    u32 address;
    u32 value;
    std::string cheat_line;
};

/// Implements support for Gateway (GateShark) cheats.
class Cheat {
public:
    explicit Cheat(std::string name = "", std::vector<CheatLine> cheat_lines = {},
                   bool enabled = false) {
        this->name = std::move(name);
        this->cheat_lines = std::move(cheat_lines);
        this->enabled = enabled;
    }

    void Execute();
    std::string ToString();

    bool GetEnabled() const {
        return enabled;
    }

    void SetEnabled(bool enabled) {
        this->enabled = enabled;
    }

    const std::vector<CheatLine>& GetCheatLines() const {
        return cheat_lines;
    }

    void SetCheatLines(std::vector<CheatLine> new_lines) {
        cheat_lines = std::move(new_lines);
    }

    const std::string& GetName() const {
        return name;
    }

    void SetName(std::string new_name) {
        name = std::move(new_name);
    }

private:
    bool enabled{};
    std::vector<CheatLine> cheat_lines;
    std::string name;
};

class CheatManager {
public:
    CheatManager();
    ~CheatManager();

    void Run();
    void RefreshCheats();
    void Save(std::vector<Cheat> cheats);

private:
    void CheatTickCallback(int);

    std::vector<Cheat> cheats_list;
    CoreTiming::EventType* tick_event;
};

std::vector<Cheat> GetCheatsFromFile();

} // namespace CheatCore
