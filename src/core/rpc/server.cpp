// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <functional>
#include "core/core.h"
#include "core/rpc/rpc_server.h"
#include "core/rpc/server.h"

namespace RPC {

Server::Server(RPCServer& rpc_server) : rpc_server{rpc_server} {}

void Server::Start() {
    const auto callback{[this](std::unique_ptr<RPC::Packet> new_request) {
        NewRequestCallback(std::move(new_request));
    }};

    try {
        zmq_server = std::make_unique<ZMQServer>(callback);
    } catch (...) {
        LOG_ERROR(RPC, "Error starting ZeroMQ server");
    }
}

void Server::Stop() {
    zmq_server.reset();
}

void Server::NewRequestCallback(std::unique_ptr<RPC::Packet> new_request) {
    if (new_request) {
        LOG_INFO(RPC, "Received request (version={}, id={}, type={}, size={})",
                 new_request->GetVersion(), new_request->GetId(),
                 static_cast<u32>(new_request->GetPacketType()), new_request->GetPacketDataSize());
    }
    rpc_server.QueueRequest(std::move(new_request));
}

} // namespace RPC
