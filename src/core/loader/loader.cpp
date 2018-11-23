// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/hle/kernel/process.h"
#include "core/loader/3dsx.h"
#include "core/loader/elf.h"
#include "core/loader/ncch.h"

namespace Loader {

const std::initializer_list<Kernel::AddressMapping> default_address_mappings{
    {0x1FF50000, 0x8000, true},                  // part of DSP RAM
    {0x1FF70000, 0x8000, true},                  // part of DSP RAM
    {0x1F000000, Memory::VRAM_N3DS_SIZE, false}, // entire VRAM
};

FileType IdentifyFile(FileUtil::IOFile& file) {
    FileType type;
#define CHECK_TYPE(loader)                                                                         \
    type = ProgramLoader_##loader::IdentifyType(file);                                             \
    if (FileType::Error != type)                                                                   \
        return type;
    CHECK_TYPE(THREEDSX)
    CHECK_TYPE(ELF)
    CHECK_TYPE(NCCH)
#undef CHECK_TYPE
    return FileType::Unknown;
}

FileType IdentifyFile(const std::string& file_name) {
    FileUtil::IOFile file{file_name, "rb"};
    if (!file.IsOpen()) {
        LOG_ERROR(Loader, "Failed to load file {}", file_name);
        return FileType::Unknown;
    }
    return IdentifyFile(file);
}

FileType GuessFromExtension(const std::string& extension_) {
    auto extension{Common::ToLower(extension_)};
    if (extension == ".elf" || extension == ".axf")
        return FileType::ELF;
    if (extension == ".cci" || extension == ".3ds")
        return FileType::CCI;
    if (extension == ".cxi" || extension == ".app")
        return FileType::CXI;
    if (extension == ".3dsx")
        return FileType::THREEDSX;
    if (extension == ".cia")
        return FileType::CIA;
    return FileType::Unknown;
}

const char* GetFileTypeString(FileType type) {
    switch (type) {
    case FileType::CCI:
        return "NCSD";
    case FileType::CXI:
        return "NCCH";
    case FileType::CIA:
        return "CIA";
    case FileType::ELF:
        return "ELF";
    case FileType::THREEDSX:
        return "3DSX";
    case FileType::Error:
    case FileType::Unknown:
        break;
    }
    return "unknown";
}

/**
 * Get a loader for a file with a specific type
 * @param file The file to load
 * @param type The type of the file
 * @param filename the file name (without path)
 * @param filepath the file full path (with name)
 * @return std::unique_ptr<ProgramLoader> a pointer to a loader object;  nullptr for unsupported
 * type
 */
static std::unique_ptr<ProgramLoader> GetFileLoader(Core::System& system, FileUtil::IOFile&& file,
                                                    FileType type, const std::string& filename,
                                                    const std::string& filepath) {
    switch (type) {
    // 3DSX file format.
    case FileType::THREEDSX:
        return std::make_unique<ProgramLoader_THREEDSX>(system, std::move(file), filename,
                                                        filepath);
    // Standard ELF file format.
    case FileType::ELF:
        return std::make_unique<ProgramLoader_ELF>(system, std::move(file), filename);
    // NCCH/NCSD container formats.
    case FileType::CXI:
    case FileType::CCI:
        return std::make_unique<ProgramLoader_NCCH>(system, std::move(file), filepath);
    default:
        return nullptr;
    }
}

std::unique_ptr<ProgramLoader> GetLoader(Core::System& system, const std::string& filename) {
    FileUtil::IOFile file{filename, "rb"};
    if (!file.IsOpen()) {
        LOG_ERROR(Loader, "Failed to load file {}", filename);
        return nullptr;
    }
    std::string filename_filename, filename_extension;
    Common::SplitPath(filename, nullptr, &filename_filename, &filename_extension);
    auto type{IdentifyFile(file)};
    auto filename_type{GuessFromExtension(filename_extension)};
    if (type != filename_type) {
        LOG_WARNING(Loader, "File {} has a different type than its extension.", filename);
        if (type == FileType::Unknown)
            type = filename_type;
    }
    LOG_DEBUG(Loader, "Loading file {} as {}...", filename, GetFileTypeString(type));
    return GetFileLoader(system, std::move(file), type, filename_filename, filename);
}

} // namespace Loader
