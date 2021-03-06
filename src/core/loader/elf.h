// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "core/loader/loader.h"

namespace Loader {

/// Loads an ELF/AXF file
class ProgramLoader_ELF final : public ProgramLoader {
public:
    explicit ProgramLoader_ELF(Core::System& system, FileUtil::IOFile&& file, std::string filename)
        : ProgramLoader{system, std::move(file)}, filename{std::move(filename)} {}

    /**
     * Returns the type of the file
     * @param file FileUtil::IOFile open file
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(FileUtil::IOFile& file);

    FileType GetFileType() override {
        return IdentifyType(file);
    }

    ResultStatus Load(Kernel::SharedPtr<Kernel::Process>& process) override;

private:
    std::string filename;
};

} // namespace Loader
