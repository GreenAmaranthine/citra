// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include "common/common_types.h"

namespace HW::AES {

enum KeySlotID : std::size_t {
    // AES keyslots used to decrypt NCCH
    NCCHSecure1 = 0x2C,
    NCCHSecure2 = 0x25,
    NCCHSecure3 = 0x18,
    NCCHSecure4 = 0x1B,

    // AES keyslot used to generate the UDS data frame CCMP key.
    UDSDataKey = 0x2D,

    // AES keyslot used for APT:Wrap/Unwrap functions
    APTWrap = 0x31,

    // AES keyslot used for decrypting ticket title key
    TicketCommonKey = 0x3D,

    // SSL Key
    SSLKey = 0x0D,

    MaxKeySlotID = 0x40,
};

constexpr std::size_t AES_BLOCK_SIZE = 16;

using AESKey = std::array<u8, AES_BLOCK_SIZE>;

void InitKeys();

void SetGeneratorConstant(const AESKey& key);
void SetKeyX(std::size_t slot_id, const AESKey& key);
void SetKeyY(std::size_t slot_id, const AESKey& key);
void SetNormalKey(std::size_t slot_id, const AESKey& key);

bool IsNormalKeyAvailable(std::size_t slot_id);
AESKey GetNormalKey(std::size_t slot_id);

void SelectCommonKeyIndex(u8 index);

} // namespace HW::AES
