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

static const std::unordered_map<u64, QStringList> issues_map{{
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

    // Test Program (CTRAging)
    {0x000400000F980000, {"Crashes Citra when booting"}},

    // Animal Crossing: Happy Home Designer (EUR)
    {0x000400000014F200, {"Needs Nintendo Network support"}},

    // Animal Crossing: New Leaf - Welcome amiibo (EUR)
    {0x0004000000198F00, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Super Mario Maker (EUR)
    {0x00040000001A0500, {"Needs Nintendo Network support"}},

    // Animal Crossing: New Leaf (EUR)
    {0x0004000000086400, {"Needs Microphone", "Needs Nintendo Network support"}},

    // nintendogs + cats (Golden Retriever & New Friends) (EUR)
    {0x0004000000030C00, {"Needs Microphone", "Needs Pedometer"}},

    // Tomodachi Life (EUR)
    {0x000400000008C400,
     {"Lip syncing glitchy when singing", GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}},

    // The Legend of Zelda: Tri Force Heroes (EUR)
    {0x0004000000177000, {"Needs DLP", "Needs Nintendo Network support"}},

    // Photos with Mario (EUR)
    {0x0004000000130600, {GitHubIssue("citra-emu/citra", 3347)}},

    // Mario Kart 7 (EUR)
    {0x0004000000030700, {"Needs DLP", "Needs Nintendo Network support"}},

    // nintendogs + cats (Toy Poodle & New Friends) (EUR)
    {0x0004000000031600, {"Needs Microphone", "Needs Pedometer"}},

    // Teenage Mutant Ninja Turtles: Master Splinter's Training Pack (EUR)
    {0x0004000000170B00, {"Softlocks when selecting a game"}},

    // Captain Toad: Treasure Tracker (EUR)
    {0x00040000001CB200, {"Needs Microphone"}},

    // Mii Maker (EUR)
    {0x0004001000022700,
     {GitHubIssue("citra-emu/citra", 3897), GitHubIssue("citra-emu/citra", 3903)}},

    // Download Play (EUR)
    {0x0004001000022100, {"Needs DLP"}},

    // Nintendo Network ID Settings (EUR)
    {0x000400100002C100, {"Needs Nintendo Network support"}},

    // Super Mario Maker (USA)
    {0x00040000001A0400, {"Needs Nintendo Network support"}},

    // Animal Crossing: Happy Home Designer (USA)
    {0x000400000014F100, {"Needs Nintendo Network support"}},

    // Photos with Mario (USA)
    {0x0004000000130500, {GitHubIssue("citra-emu/citra", 3348)}},

    // Animal Crossing: New Leaf (USA)
    {0x0004000000086300, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Tomodachi Life (USA)
    {0x000400000008C300,
     {"Lip syncing glitchy when singing", GitHubIssue("citra-emu/citra", 4320),
      "Exchange Miis or Other Items - The name of the island flashes",
      "Scan QR Code - Preparing camera never ends"}},

    // Mario Kart 7 (USA)
    {0x0004000000030800, {"Needs DLP", "Needs Nintendo Network support"}},

    // Minecraft (USA)
    {0x00040000001B8700, {"Softlock when saving"}},

    // Sonic Generations (USA)
    {0x0004000000062300, {"Multiplayer needs deprecated UDS"}},

    // Luigi's Mansion (USA)
    {0x00040000001D1900, {GitHubIssue("citra-emu/citra", 4330)}},

    // Swapdoodle (USA)
    {0x00040000001A2D00, {GitHubIssue("citra-emu/citra", 4388)}},

    // とびだせ どうぶつの森 (JPN)
    {0x0004000000086200, {"Needs Microphone"}},

    // とびだせ どうぶつの森 amiibo+ (JPN)
    {0x0004000000198D00, {"Needs Microphone", "Needs Nintendo Network support"}},

    // スーパーマリオメーカー for ニンテンドー3DS (JPN)
    {0x00040000001A0300, {"Needs Nintendo Network support"}},

    // 튀어나와요 동물의 숲 (KOR)
    {0x0004000000086500, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Animal Crossing: New Leaf - Welcome amiibo (KOR)
    {0x0004000000199000, {"Needs Microphone", "Needs Nintendo Network support"}},

    // Super Mario Maker (KOR)
    {0x00040000001BB800, {"Needs Nintendo Network support"}},
}};
