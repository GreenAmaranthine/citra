// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <unordered_map>
#include <QString>
#include "common/common_types.h"

static inline QString GitHubIssue(const QString& repo, int number) {
    return QString("<a href=\"https://github.com/%1/issues/%2\"><span style=\"text-decoration: "
                   "underline; color:#039be5;\">%1#%2</span></a>")
        .arg(repo, QString::number(number));
}

static const std::unordered_map<u64, QStringList> compatibility_database{{
    // FBI
    {0x000400000F800100, {GitHubIssue("citra-valentin/citra", 451)}},

    // DSiMenuPlusPlus
    {0x0004000004395500, {"Citra crashes with Unknown result status"}},

    // White Haired Cat Girl
    {0x000400000D5CDB00, {"Needs SOC:GetAddrInfo"}},

    // Boot NTR Selector
    {0x000400000EB00000, {"Load fails (NTR is already running)"}},

    // Detective Pikachu
    {0x00040000001C1E00, {GitHubIssue("citra-emu/citra", 3612)}},

    // Pokémon Sun
    {0x0004000000164800, {GitHubIssue("citra-emu/citra", 2311), "Needs Nintendo Network support"}},

    // Pokémon Moon
    {0x0004000000175E00, {GitHubIssue("citra-emu/citra", 2311), "Needs Nintendo Network support"}},

    // Pokémon Omega Ruby
    {0x000400000011C400, {GitHubIssue("citra-emu/citra", 4092), "Needs Nintendo Network support"}},

    // Pokémon Alpha Sapphire
    {0x000400000011C500, {GitHubIssue("citra-emu/citra", 4092), "Needs Nintendo Network support"}},

    // Pokémon Ultra Sun
    {0x00040000001B5000, {"Needs Nintendo Network support"}},

    // Pokémon Ultra Moon
    {0x00040000001B5100, {"Needs Nintendo Network support"}},

    // Pokémon X
    {0x0004000000055D00, {GitHubIssue("citra-emu/citra", 3009)}},

    // Pokémon Y
    {0x0004000000055E00, {GitHubIssue("citra-emu/citra", 3009)}},

    // Super Mario World
    {0x000400000F700E00, {}},

    // Test Program (CTRAging)
    {0x000400000F980000, {"Crashes Citra when booting"}},

    // Ever Oasis (EUR)
    {0x00040000001A4900, {}},

    // Fire Emblem Warriors (EUR)
    {0x000400000F70CD00, {}},

    // Animal Crossing: Happy Home Designer (EUR)
    {0x000400000014F200, {"Needs Nintendo Network support"}},

    // Hyrule Warriors Legends (EUR)
    {0x000400000017EB00, {}},

    // Luigi's Mansion 2 (EUR)
    {0x0004000000076500, {}},

    // Monster Hunter Stories (EUR)
    {0x00040000001BC600, {}},

    // Animal Crossing: New Leaf - Welcome amiibo (EUR)
    {0x0004000000198F00, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Super Mario Maker for Nintendo 3DS (EUR)
    {0x00040000001A0500, {"Needs Nintendo Network support"}},

    // Animal Crossing: New Leaf (EUR)
    {0x0004000000086400, {"Needs Microphone", "Needs Nintendo Network support"}},

    // nintendogs + cats (Golden Retriever & New Friends) (EUR)
    {0x0004000000030C00, {"Needs Microphone", "Needs Pedometer"}},

    // The Legend of Zelda: Ocarina of Time 3D (EUR)
    {0x0004000000033600, {}},

    // Poochy & Yoshi's Woolly World (EUR)
    {0x00040000001A4200, {}},

    // Super Mario 3D Land (EUR)
    {0x0004000000053F00, {}},

    // Tomodachi Life (EUR)
    {0x000400000008C400,
     {GitHubIssue("citra-valentin/citra", 277), GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}},

    // The Legend of Zelda: Tri Force Heroes (EUR)
    {0x0004000000177000, {"Needs DLP", "Needs Nintendo Network support"}},

    // New Super Mario Bros. 2 (EUR)
    {0x000400000007AF00, {}},

    // New Super Mario Bros. 2: Special Edition (EUR)
    {0x0004000000137F00, {}},

    // Miitopia (EUR)
    {0x00040000001B4F00, {}},

    // PICROSS e (EUR)
    {0x00040000000A9200, {}},

    // PICROSS e2 (EUR)
    {0x00040000000CBC00, {}},

    // PICROSS e3 (EUR)
    {0x0004000000102900, {}},

    // PICROSS e4 (EUR)
    {0x0004000000128400, {}},

    // PICROSS e5 (EUR)
    {0x000400000014D200, {}},

    // PICROSS e6 (EUR)
    {0x000400000016E800, {}},

    // PICROSS e7 (EUR)
    {0x00040000001AD600, {}},

    // PICROSS e8 (EUR)
    {0x00040000001CE000, {}},

    // Photos with Mario (EUR)
    {0x0004000000130600, {GitHubIssue("citra-emu/citra", 3347)}},

    // Mario Kart 7 (EUR)
    {0x0004000000030700, {"Needs DLP", "Needs Nintendo Network support"}},

    // 2048 (EUR)
    {0x0004000000143D00, {}},

    // Picross 3D: Round 2 (EUR)
    {0x0004000000187E00, {}},

    // Super Smash Bros. for Nintendo 3DS (EUR)
    {0x00040000000EE000, {}},

    // The Legend of Zelda: Majora's Mask 3D (EUR)
    {0x0004000000125600, {}},

    // Kirby: Triple Deluxe (EUR)
    {0x000400000010C000, {}},

    // YO-KAI WATCH 2: Psychic Specters (EUR)
    {0x00040000001B2900, {}},

    // Captain Toad: Treasure Tracker (EUR)
    {0x00040000001CB200, {}},

    // Cut the Rope: Triple Treat (EUR)
    {0x0004000000116700, {}},

    // Kid Icarus Uprising (EUR)
    {0x0004000000030200, {}},

    // Paper Mario Sticker Star (EUR)
    {0x00040000000A5F00, {}},

    // nintendogs + cats (Toy Poodle & New Friends) (EUR)
    {0x0004000000031600, {"Needs Microphone", "Needs Pedometer"}},

    // Teenage Mutant Ninja Turtles: Master Splinter's Training Pack (EUR)
    {0x0004000000170B00, {"Softlocks when selecting a game"}},

    // Captain Toad: Treasure Tracker (EUR)
    {0x00040000001CB200, {"Needs Microphone"}},

    // ??? (EUR)
    {0x0004001000022A00, {}},

    // Mii Maker (EUR)
    {0x0004001000022700,
     {GitHubIssue("citra-emu/citra", 3897), GitHubIssue("citra-emu/citra", 3903)}},

    // Download Play (EUR)
    {0x0004001000022100, {"Needs DLP"}},

    // Health and Safety Information (EUR)
    {0x0004001000022300, {}},

    // Nintendo Network ID Settings (EUR)
    {0x000400100002C100, {"Needs Nintendo Network support"}},

    // Captain Toad Demo (EUR)
    {0x00040002001CB201, {}},

    // PICROSS e (USA)
    {0x00040000000E5D00, {}},

    // PICROSS e2 (USA)
    {0x00040000000CD400, {}},

    // PICROSS e3 (USA)
    {0x0004000000101D00, {}},

    // PICROSS e4 (USA)
    {0x0004000000127300, {}},

    // PICROSS e5 (USA)
    {0x0004000000149800, {}},

    // PICROSS e6 (USA)
    {0x000400000016EF00, {}},

    // PICROSS e7 (USA)
    {0x00040000001ADB00, {}},

    // PICROSS e8 (USA)
    {0x00040000001CF700, {}},

    // Super Mario Maker for Nintendo 3DS (USA)
    {0x00040000001A0400, {"Needs Nintendo Network support"}},

    // Animal Crossing: Happy Home Designer (USA)
    {0x000400000014F100, {"Needs Nintendo Network support"}},

    // Moco Moco Friends (USA)
    {0x000400000017F200, {}},

    // Miitopia (USA)
    {0x00040000001B4E00, {}},

    // Photos with Mario (USA)
    {0x0004000000130500, {GitHubIssue("citra-emu/citra", 3348)}},

    // Animal Crossing: New Leaf (USA)
    {0x0004000000086300, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Tomodachi Life (USA)
    {0x000400000008C300,
     {GitHubIssue("citra-valentin/citra", 277), GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}},

    // Mario Kart 7 (USA)
    {0x0004000000030800, {"Needs DLP", "Needs Nintendo Network support"}},

    // 2048 (USA)
    {0x0004000000139000, {}},

    // YO-KAI WATCH 2: Psychic Specters (USA)
    {0x00040000001B2700, {}},

    // Cut the Rope: Triple Treat (USA)
    {0x0004000000112600, {}},

    // Minecraft (USA)
    {0x00040000001B8700, {"Softlock when saving"}},

    // Sonic Generations (USA)
    {0x0004000000062300, {"Multiplayer needs deprecated UDS"}},

    // MONSTER HUNTER 4 ULTIMATE (USA)
    {0x0004000000126300, {}},

    // Luigi's Mansion (USA)
    {0x00040000001D1900, {GitHubIssue("citra-emu/citra", 4330)}},

    // Captain Toad Demo (USA)
    {0x00040002001CB101, {}},

    // Swapdoodle (USA)
    {0x00040000001A2D00, {GitHubIssue("citra-emu/citra", 4388)}},

    // Miitopia(ミートピア) (JPN)
    {0x0004000000178800, {}},

    // My Melody Negai ga Kanau Fushigi na Hako (JPN)
    {0x0004000000181000, {}},

    // とびだせ どうぶつの森 (JPN)
    {0x0004000000086200, {"Needs Microphone"}},

    // とびだせ どうぶつの森 amiibo+ (JPN)
    {0x0004000000198D00, {"Needs Microphone", "Needs Nintendo Network support"}},

    // スーパーマリオメーカー for ニンテンドー3DS (JPN)
    {0x00040000001A0300, {"Needs Nintendo Network support"}},

    // マリオカート7 (JPN)
    {0x0004000000030600, {}},

    // 2048 (JPN)
    {0x000400000014AF00, {}},

    // Captain Toad Demo (JPN)
    {0x00040002001CB001, {}},

    // 튀어나와요 동물의 숲 (KOR)
    {0x0004000000086500, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Animal Crossing: New Leaf - Welcome amiibo (KOR)
    {0x0004000000199000, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Super Mario Maker for Nintendo 3DS (KOR)
    {0x00040000001BB800, {"Needs Nintendo Network support"}},
}};
