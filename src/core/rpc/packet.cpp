// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstring>
#include "core/rpc/packet.h"

namespace RPC {

Packet::Packet(const PacketHeader& header, u8* data,
               std::function<void(Packet&)> send_reply_callback)
    : header{header}, send_reply_callback{std::move(send_reply_callback)} {

    packet_data.resize(header.packet_size);
    std::memcpy(packet_data.data(), data, header.packet_size);
}

} // namespace RPC
