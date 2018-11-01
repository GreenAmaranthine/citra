// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <unordered_map>
#include <QString>
#include "common/common_types.h"

static QString GitHubIssue(const QString& repo, int number) {
    return QString(
               "<a href=\"https://github.com/repo/issues/number\"><span style=\"text-decoration: "
               "underline; color:#039be5;\">repo#number</span></a>")
        .replace("repo", repo)
        .replace("number", QString::number(number));
}

static const std::unordered_map<u64, QStringList> compatibility_database{{
    // Homebrew
    {0x000400000F800100, {GitHubIssue("citra-valentin/citra", 451)}},   // FBI
    {0x0004000004395500, {"Citra crashes with Unknown result status"}}, // DSiMenuPlusPlus
    {0x000400000D5CDB00, {"Needs SOC:GetAddrInfo"}},                    // White Haired Cat Girl
    {0x000400000EB00000, {"Load fails (NTR is already running)"}},      // Boot NTR Selector

    // ALL
    {0x00040000001C1E00, {GitHubIssue("citra-emu/citra", 3612)}}, // Detective Pikachu
    {0x0004000000164800,
     {GitHubIssue("citra-emu/citra", 2311), "Needs Nintendo Network support"}}, // Pokémon Sun
    {0x0004000000175E00,
     {GitHubIssue("citra-emu/citra", 2311), "Needs Nintendo Network support"}}, // Pokémon Moon
    {0x000400000011C400,
     {GitHubIssue("citra-emu/citra", 4092),
      "Needs Nintendo Network support"}}, // Pokémon Omega Ruby
    {0x000400000011C500,
     {GitHubIssue("citra-emu/citra", 4092),
      "Needs Nintendo Network support"}},                         // Pokémon Alpha Sapphire
    {0x00040000001B5000, {"Needs Nintendo Network support"}},     // Pokémon Ultra Sun
    {0x00040000001B5100, {"Needs Nintendo Network support"}},     // Pokémon Ultra Moon
    {0x0004000000055D00, {GitHubIssue("citra-emu/citra", 3009)}}, // Pokémon X
    {0x0004000000055E00, {GitHubIssue("citra-emu/citra", 3009)}}, // Pokémon Y
    {0x000400000F700E00, {}},                                     // Super Mario World
    {0x000400000F980000, {"Crashes Citra when booting"}},         // Test Program (CTRAging)

    // EUR
    {0x00040000001A4900, {}}, // Ever Oasis
    {0x000400000F70CD00, {}}, // Fire Emblem Warriors
    {0x000400000014F200,
     {"Needs Nintendo Network support"}}, // Animal Crossing: Happy Home Designer
    {0x000400000017EB00, {}},             // Hyrule Warriors Legends
    {0x0004000000076500, {}},             // Luigi’s Mansion 2
    {0x00040000001BC600, {}},             // Monster Hunter Stories
    {0x0004000000198F00,
     {"Needs Nintendo Network support"}}, // Animal Crossing: New Leaf - Welcome amiibo
    {0x00040000001A0500, {"Needs Nintendo Network support"}}, // Super Mario Maker for Nintendo 3DS
    {0x0004000000086400, {"Needs Nintendo Network support"}}, // Animal Crossing: New Leaf
    {0x0004000000030C00,
     {"Needs Microphone", "Needs Pedometer"}}, // nintendogs + cats (Golden Retriever & New Friends)
    {0x0004000000033600, {}},                  // The Legend of Zelda: Ocarina of Time 3D
    {0x00040000001A4200, {}},                  // Poochy & Yoshi's Woolly World
    {0x0004000000053F00, {}},                  // Super Mario 3D Land
    {0x000400000008C400,
     {GitHubIssue("citra-valentin/citra", 277), GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}}, // Tomodachi Life
    {0x0004000000177000,
     {"Needs DLP", "Needs Nintendo Network support"}}, // The Legend of Zelda: Tri Force Heroes
    {0x000400000007AF00, {}},                          // New Super Mario Bros. 2
    {0x0004000000137F00, {}},                          // New Super Mario Bros. 2: Special Edition
    {0x00040000001B4F00, {}},                          // Miitopia
    {0x00040000000A9200, {}},                          // PICROSS e
    {0x00040000000CBC00, {}},                          // PICROSS e2
    {0x0004000000102900, {}},                          // PICROSS e3
    {0x0004000000128400, {}},                          // PICROSS e4
    {0x000400000014D200, {}},                          // PICROSS e5
    {0x000400000016E800, {}},                          // PICROSS e6
    {0x00040000001AD600, {}},                          // PICROSS e7
    {0x00040000001CE000, {}},                          // PICROSS e8
    {0x0004000000130600, {GitHubIssue("citra-emu/citra", 3347)}}, // Photos with Mario
    {0x0004000000030700,
     {"Local multiplayer needs DLP", "Needs Nintendo Network support"}}, // Mario Kart 7
    {0x0004000000143D00, {}},                                            // 2048
    {0x0004000000187E00, {}},                                            // Picross 3D: Round 2
    {0x00040000000EE000, {}}, // Super Smash Bros. for Nintendo 3DS
    {0x0004000000125600, {}}, // The Legend of Zelda: Majora's Mask 3D
    {0x000400000010C000, {}}, // Kirby: Triple Deluxe
    {0x00040000001B2900, {}}, // YO-KAI WATCH 2: Psychic Specters
    {0x00040000001CB200, {}}, // Captain Toad: Treasure Tracker
    {0x0004000000116700, {}}, // Cut the Rope: Triple Treat
    {0x0004000000030200, {}}, // Kid Icarus Uprising
    {0x00040000000A5F00, {}}, // Paper Mario Sticker Star
    {0x0004000000031600,
     {"Needs Microphone", "Needs Pedometer"}}, // nintendogs + cats (Toy Poodle & New Friends)
    {0x0004000000170B00, {"Softlocks when selecting a game"}}, // Teenage Mutant Ninja Turtles:
                                                               // Master Splinter's Training Pack

    // EUR (System)
    {0x0004001000022A00, {}}, // ???
    {0x0004001000022700,
     {GitHubIssue("citra-emu/citra", 3897), GitHubIssue("citra-emu/citra", 3903)}}, // Mii Maker
    {0x0004001000022100, {"Needs DLP"}},                                            // Download Play
    {0x0004001000022300, {}},                              // Health and Safety Information
    {0x000400100002C100, {"Citra crashes when stopping"}}, // Nintendo Network ID Settings

    // EUR (Demos)
    {0x00040002001CB201, {}}, // Captain Toad Demo

    // USA
    {0x00040000000E5D00, {}},                                 // PICROSS e
    {0x00040000000CD400, {}},                                 // PICROSS e2
    {0x0004000000101D00, {}},                                 // PICROSS e3
    {0x0004000000127300, {}},                                 // PICROSS e4
    {0x0004000000149800, {}},                                 // PICROSS e5
    {0x000400000016EF00, {}},                                 // PICROSS e6
    {0x00040000001ADB00, {}},                                 // PICROSS e7
    {0x00040000001CF700, {}},                                 // PICROSS e8
    {0x00040000001A0400, {"Needs Nintendo Network support"}}, // Super Mario Maker for Nintendo 3DS
    {0x000400000014F100,
     {"Needs Nintendo Network support"}}, // Animal Crossing: Happy Home Designer
    {0x000400000017F200, {}},             // Moco Moco Friends
    {0x00040000001B4E00, {}},             // Miitopia
    {0x0004000000130500, {GitHubIssue("citra-emu/citra", 3348)}}, // Photos with Mario
    {0x0004000000086300, {"Needs Nintendo Network support"}},     // Animal Crossing: New Leaf
    {0x000400000008C300,
     {GitHubIssue("citra-valentin/citra", 277), GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}}, // Tomodachi Life
    {0x0004000000030800,
     {"Local multiplayer needs DLP", "Needs Nintendo Network support"}}, // Mario Kart 7
    {0x0004000000139000, {}},                                            // 2048
    {0x00040000001B2700, {}},                                   // YO-KAI WATCH 2: Psychic Specters
    {0x0004000000112600, {}},                                   // Cut the Rope: Triple Treat
    {0x00040000001B8700, {}},                                   // Minecraft
    {0x0004000000062300, {"Multiplayer needs deprecated UDS"}}, // Sonic Generations
    {0x0004000000126300, {}},                                   // MONSTER HUNTER 4 ULTIMATE
    {0x00040000001D1900, {GitHubIssue("citra-emu/citra", 4330)}}, // Luigi's Mansion

    // USA (Demos)
    {0x00040002001CB101, {}}, // Captain Toad Demo

    // JPN
    {0x0004000000178800, {}}, // Miitopia(ミートピア)
    {0x0004000000181000, {}}, // My Melody Negai ga Kanau Fushigi na Hako
    {0x0004000000086200, {}}, // とびだせ どうぶつの森
    {0x0004000000198D00, {"Needs Nintendo Network support"}}, // とびだせ どうぶつの森 amiibo+
    {0x00040000001A0300,
     {"Needs Nintendo Network support"}}, // スーパーマリオメーカー for ニンテンドー3DS
    {0x0004000000030600, {}},             // マリオカート7
    {0x000400000014AF00, {}},             // 2048

    // JPN (Demos)
    {0x00040002001CB001, {}}, // Captain Toad Demo

    // KOR
    {0x0004000000086500, {"Needs Nintendo Network support"}}, // 튀어나와요 동물의 숲
    {0x0004000000199000,
     {"Needs Nintendo Network support"}}, // Animal Crossing: New Leaf - Welcome amiibo
    {0x00040000001BB800, {"Needs Nintendo Network support"}}, // Super Mario Maker for Nintendo 3DS
}};
