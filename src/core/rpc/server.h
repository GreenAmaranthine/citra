// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

namespace RPC {

class Packet;
class RPCServer;

class Server {
public:
    Server(RPCServer& rpc_server);
    void Start();
    void Stop();
    void NewRequestCallback(std::unique_ptr<RPC::Packet> new_request);

private:
    struct Impl;
    std::shared_ptr<Impl> impl;

    RPCServer& rpc_server;
};

} // namespace RPC
