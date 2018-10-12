// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/hle/service/service.h"

namespace Service::FS {

struct ExtSaveDataInfo {
    u8 media_type;
    u8 unknown;
    u16 reserved;
    u64 save_id;
    u32 reserved2;
};

struct SystemSaveDataInfo {
    u8 media_type;
    u8 unknown;
    u16 reserved;
    u32 save_id;
};

class FS_USER final : public ServiceFramework<FS_USER> {
public:
    FS_USER();

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void OpenFile(Kernel::HLERequestContext& ctx);
    void OpenFileDirectly(Kernel::HLERequestContext& ctx);
    void DeleteFile(Kernel::HLERequestContext& ctx);
    void RenameFile(Kernel::HLERequestContext& ctx);
    void DeleteDirectory(Kernel::HLERequestContext& ctx);
    void DeleteDirectoryRecursively(Kernel::HLERequestContext& ctx);
    void CreateFile(Kernel::HLERequestContext& ctx);
    void CreateDirectory(Kernel::HLERequestContext& ctx);
    void RenameDirectory(Kernel::HLERequestContext& ctx);
    void OpenDirectory(Kernel::HLERequestContext& ctx);
    void OpenArchive(Kernel::HLERequestContext& ctx);
    void CloseArchive(Kernel::HLERequestContext& ctx);
    void IsSdmcDetected(Kernel::HLERequestContext& ctx);
    void IsSdmcWriteable(Kernel::HLERequestContext& ctx);
    void FormatSaveData(Kernel::HLERequestContext& ctx);
    void FormatThisUserSaveData(Kernel::HLERequestContext& ctx);
    void GetFreeBytes(Kernel::HLERequestContext& ctx);
    void CreateExtSaveData(Kernel::HLERequestContext& ctx);
    void DeleteExtSaveData(Kernel::HLERequestContext& ctx);
    void CardSlotIsInserted(Kernel::HLERequestContext& ctx);
    void DeleteSystemSaveData(Kernel::HLERequestContext& ctx);
    void CreateSystemSaveData(Kernel::HLERequestContext& ctx);
    void CreateLegacySystemSaveData(Kernel::HLERequestContext& ctx);
    void InitializeWithSdkVersion(Kernel::HLERequestContext& ctx);
    void SetPriority(Kernel::HLERequestContext& ctx);
    void GetPriority(Kernel::HLERequestContext& ctx);
    void GetArchiveResource(Kernel::HLERequestContext& ctx);
    void GetFormatInfo(Kernel::HLERequestContext& ctx);
    void GetProgramLaunchInfo(Kernel::HLERequestContext& ctx);
    void ObsoletedCreateExtSaveData(Kernel::HLERequestContext& ctx);
    void ObsoletedDeleteExtSaveData(Kernel::HLERequestContext& ctx);
    void GetNumSeeds(Kernel::HLERequestContext& ctx);
    void AddSeed(Kernel::HLERequestContext& ctx);
    void SetSaveDataSecureValue(Kernel::HLERequestContext& ctx);
    void GetSaveDataSecureValue(Kernel::HLERequestContext& ctx);
    void GetThisSaveDataSecureValue(Kernel::HLERequestContext& ctx);
    void EnumerateExtSaveData(Kernel::HLERequestContext& ctx);
    void EnumerateSystemSaveData(Kernel::HLERequestContext& ctx);
    void GetSdmcCid(Kernel::HLERequestContext& ctx);
    void GetNandCid(Kernel::HLERequestContext& ctx);
    void ReadExtSaveDataIcon(Kernel::HLERequestContext& ctx);
    void GetCardType(Kernel::HLERequestContext& ctx);
    void DeleteSdmcRoot(Kernel::HLERequestContext& ctx);
    void InitializeCtrFileSystem(Kernel::HLERequestContext& ctx);
    void GetSdmcCtrRootPath(Kernel::HLERequestContext& ctx);
    void CheckUpdatedDat(Kernel::HLERequestContext& ctx);

    u32 priority = -1; ///< For SetPriority and GetPriority service functions
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::FS
