// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <getopt.h>
#include <glad/glad.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef _WIN32
// windows.h needs to be included before shellapi.h
#include <windows.h>

#include <shellapi.h>
#endif

#include "common/common_types.h"
#include "common/scm_rev.h"
#include "core/announce_multiplayer_session.h"
#include "core/core.h"
#include "core/settings.h"
#include "network/room.h"

static void PrintHelp(const char* argv0) {
    std::cout << "Usage: " << argv0
              << " [options] <filename>\n"
                 "--room-name         The name of the room\n"
                 "--room-description  The room description\n"
                 "--port              The port used for the room\n"
                 "--max_members       The maximum number of members for this room\n"
                 "--password          The password for the room\n"
                 "-h, --help          Display this help and exit\n"
                 "-v, --version       Output version information and exit\n";
}

static void PrintVersion() {
    std::cout << "Citra dedicated room " << Common::g_scm_branch << " " << Common::g_scm_desc
              << " Libnetwork: " << Network::network_version << std::endl;
}

/// Program entry point
int main(int argc, char** argv) {
    int option_index{};
    char* endarg;
    // This is just to be able to link against core
    gladLoadGL();
    std::string room_name, room_description, creator, password;
    u32 port{Network::DefaultRoomPort}, max_members{16};
    static struct option long_options[]{
        {"room-name", required_argument, 0, 'n'},
        {"room-description", required_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {"max-members", required_argument, 0, 'm'},
        {"password", required_argument, 0, 'w'},
        {"creator", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0},
    };
    while (optind < argc) {
        int arg{getopt_long(argc, argv, "n:d:p:m:w:c:hv", long_options, &option_index)};
        if (arg != -1) {
            switch (arg) {
            case 'n':
                room_name.assign(optarg);
                break;
            case 'd':
                room_description.assign(optarg);
                break;
            case 'p':
                port = strtoul(optarg, &endarg, 0);
                break;
            case 'm':
                max_members = strtoul(optarg, &endarg, 0);
                break;
            case 'w':
                password.assign(optarg);
                break;
            case 'c':
                creator.assign(optarg);
                break;
            case 'h':
                PrintHelp(argv[0]);
                return 0;
            case 'v':
                PrintVersion();
                return 0;
            }
        }
    }
    if (room_name.empty()) {
        std::cout << "Room name is empty!\n\n";
        PrintHelp(argv[0]);
        return -1;
    }
    if (max_members >= Network::MaxConcurrentConnections || max_members < 2) {
        std::cout << "max-members needs to be in the range 2 - "
                  << Network::MaxConcurrentConnections << "!\n\n";
        PrintHelp(argv[0]);
        return -1;
    }
    if (port > 65535) {
        std::cout << "port needs to be in the range 0 - 65535!\n\n";
        PrintHelp(argv[0]);
        return -1;
    }
    Network::Room room;
    if (!room.Create(room_name, room_description, creator, port, password, max_members)) {
        std::cout << "Failed to create room!\n\n";
        return -1;
    }
    std::cout << "Room is open. Close with Q+Enter...\n\n";
    auto announce_session{std::make_unique<Core::AnnounceMultiplayerSession>(room)};
    announce_session->Start();
    while (room.IsOpen()) {
        std::string in;
        std::cin >> in;
        if (in.size() > 0) {
            announce_session->Stop();
            announce_session.reset();
            room.Destroy();
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    announce_session->Stop();
    announce_session.reset();
    room.Destroy();
    return 0;
}
