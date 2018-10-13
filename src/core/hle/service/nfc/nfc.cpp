// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/file_util.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/nfc/nfc.h"
#include "core/hle/service/nfc/nfc_m.h"
#include "core/hle/service/nfc/nfc_u.h"

namespace Service::NFC {

#define AMIIBO_YEAR_FROM_DATE(dt) ((dt >> 1) + 2000)
#define AMIIBO_MONTH_FROM_DATE(dt) ((*((&dt) + 1) >> 5) & 0xF)
#define AMIIBO_DAY_FROM_DATE(dt) (*((&dt) + 1) & 0x1F)

struct TagInfo {
    u16_le id_offset_size; /// "u16 size/offset of the below ID data. Normally this is 0x7. When
                           /// this is <=10, this field is the size of the below ID data. When this
                           /// is >10, this is the offset of the 10-byte ID data, relative to
                           /// structstart+4+<offsetfield-10>. It's unknown in what cases this
                           /// 10-byte ID data is used."
    u8 unk_x2;             //"Unknown u8, normally 0x0."
    u8 unk_x3;             //"Unknown u8, normally 0x2."
    std::array<u8, 0x28> id; //"ID data. When the above size field is 0x7, this is the 7-byte NFC
                             // tag UID, followed by all-zeros."
};

struct AmiiboConfig {
    u16_le lastwritedate_year;
    u8 lastwritedate_month;
    u8 lastwritedate_day;
    u16_le write_counter;
    std::array<u8, 3> characterID; /// the first element is the collection ID, the second the
                                   /// character in this collection, the third the variant
    u8 series;                     /// ID of the series
    u16_le amiiboID; /// ID shared by all exact same amiibo. Some amiibo are only distinguished by
                     /// this one like regular SMB Series Mario and the gold one
    u8 type;         /// Type of amiibo 0 = figure, 1 = card, 2 = plush
    u8 pagex4_byte3;
    u16_le
        appdata_size; /// "NFC module writes hard-coded u8 value 0xD8 here. This is the size of the
                      /// Amiibo AppData, apps can use this with the AppData R/W commands. ..."
    INSERT_PADDING_BYTES(
        0x30); /// "Unused / reserved: this is cleared by NFC module but never written after that."
};

struct AmiiboSettings {
    std::array<u8, 0x60> mii;        /// "Owner Mii."
    std::array<u16_le, 11> nickname; /// "UTF-16BE Amiibo nickname."
    u8 flags; /// "This is plaintext_amiibosettingsdata[0] & 0xF." See also the NFC_amiiboFlag
              /// enums.
    u8 countrycodeid; /// "This is plaintext_amiibosettingsdata[1]." "Country Code ID, from the
                      /// system which setup this amiibo."
    u16_le setupdate_year;
    u8 setupdate_month;
    u8 setupdate_day;
    INSERT_PADDING_BYTES(0x2C); // Normally all-zero?
};

struct IdentificationBlockRaw {
    u16_le char_id;
    u8 char_variant;
    u8 figure_type;
    u16_be model_number;
    u8 series;
    INSERT_PADDING_BYTES(0x2F);
};

struct IdentificationBlockReply {
    u16_le char_id;
    u8 char_variant;
    u8 series;
    u16_le model_number;
    u8 figure_type;
    INSERT_PADDING_BYTES(0x2F);
};

void Module::Interface::Initialize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x01, 1, 0};
    u8 param{rp.Pop<u8>()};

    nfc->nfc_tag_state = TagState::NotScanning;

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called, param={}", param);
}

void Module::Interface::Shutdown(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x02, 1, 0};
    u8 param{rp.Pop<u8>()};

    nfc->nfc_tag_state = TagState::NotInitialized;

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called, param={}", param);
}

void Module::Interface::StartCommunication(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0x03, 1, 0};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called");
}

void Module::Interface::StopCommunication(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0x04, 1, 0};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called");
}

void Module::Interface::StartTagScanning(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x05, 1, 0};
    u16 in_val{rp.Pop<u16>()};

    nfc->nfc_tag_state = TagState::Scanning;

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called, in_val={:04x}", in_val);
}

void Module::Interface::GetTagInfo(Kernel::HLERequestContext& ctx) {
    TagInfo tag_info{};

    nfc->mutex.lock();
    tag_info.id_offset_size = 0x7;
    tag_info.unk_x3 = 0x02;
    tag_info.id[0] = nfc->encrypted_data[0];
    tag_info.id[1] = nfc->encrypted_data[1];
    tag_info.id[2] = nfc->encrypted_data[2];
    tag_info.id[3] = nfc->encrypted_data[4];
    tag_info.id[4] = nfc->encrypted_data[5];
    tag_info.id[5] = nfc->encrypted_data[6];
    tag_info.id[6] = nfc->encrypted_data[7];
    nfc->mutex.unlock();

    IPC::ResponseBuilder rb{ctx, 0x11, (sizeof(TagInfo) / sizeof(u32)) + 1, 0};
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw<TagInfo>(tag_info);
}

void Module::Interface::GetAmiiboConfig(Kernel::HLERequestContext& ctx) {
    AmiiboConfig amiibo_config{};

    nfc->mutex.lock();
    amiibo_config.lastwritedate_year = AMIIBO_YEAR_FROM_DATE(nfc->decrypted_data[0x32]);
    amiibo_config.lastwritedate_month = AMIIBO_MONTH_FROM_DATE(nfc->decrypted_data[0x32]);
    amiibo_config.lastwritedate_day = AMIIBO_DAY_FROM_DATE(nfc->decrypted_data[0x32]);

    amiibo_config.write_counter = (nfc->decrypted_data[0xB4] << 8) | nfc->decrypted_data[0xB5];
    amiibo_config.characterID[0] = nfc->decrypted_data[0x1DC];
    amiibo_config.characterID[1] = nfc->decrypted_data[0x1DD];
    amiibo_config.characterID[2] = nfc->decrypted_data[0x1DE];
    amiibo_config.series = nfc->decrypted_data[0x1e2];
    amiibo_config.amiiboID = (nfc->decrypted_data[0x1e0] << 8) | nfc->decrypted_data[0x1e1];
    amiibo_config.type = nfc->decrypted_data[0x1DF];
    amiibo_config.pagex4_byte3 = nfc->decrypted_data[0x2B]; // raw page 0x4 byte 0x3, dec byte
    amiibo_config.appdata_size = 0xD8;
    nfc->mutex.unlock();

    IPC::ResponseBuilder rb{ctx, 0x18, 17, 0};
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw<AmiiboConfig>(amiibo_config);
}

void Module::Interface::GetAmiiboSettings(Kernel::HLERequestContext& ctx) {
    AmiiboSettings amsettings{};
    ResultCode result{RESULT_SUCCESS};

    nfc->mutex.lock();
    if (!(nfc->decrypted_data[0x2C] & 0x10)) {
        result = ResultCode(0xC8A17628); // uninitialised
    } else {
        result = RESULT_SUCCESS;
        memcpy(amsettings.mii.data(), &nfc->decrypted_data[0x4C], amsettings.mii.size());
        memcpy(amsettings.nickname.data(), &nfc->decrypted_data[0x38],
               4 * 5); // amiibo doesnt have the null terminator
        amsettings.flags = nfc->decrypted_data[0x2C] &
                           0xF; // todo: we should only load some of these values if the unused
                                // flag bits are set correctly https://3dbrew.org/wiki/Amiibo
        amsettings.countrycodeid = nfc->decrypted_data[0x2D];

        amsettings.setupdate_year = AMIIBO_YEAR_FROM_DATE(nfc->decrypted_data[0x30]);
        amsettings.setupdate_month = AMIIBO_MONTH_FROM_DATE(nfc->decrypted_data[0x30]);
        amsettings.setupdate_day = AMIIBO_DAY_FROM_DATE(nfc->decrypted_data[0x30]);
    }
    nfc->mutex.unlock();

    IPC::ResponseBuilder rb{ctx, 0x17, (sizeof(AmiiboSettings) / sizeof(u32)) + 1, 0};
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw<AmiiboSettings>(amsettings);
}

void Module::Interface::StopTagScanning(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x06, 0, 0};

    nfc->nfc_tag_state = TagState::NotScanning;

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called");
}

void Module::Interface::LoadAmiiboData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x07, 0, 0};

    nfc->nfc_tag_state = TagState::TagDataLoaded;

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NFC, "(STUBBED) called");
}

void Module::Interface::ResetTagScanState(Kernel::HLERequestContext& ctx) {
    nfc->nfc_tag_state = TagState::TagInRange;

    IPC::ResponseBuilder rb{ctx, 0x08, 1, 0};
    rb.Push(RESULT_SUCCESS);
}

void Module::Interface::GetTagInRangeEvent(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0x0B, 1, 2};
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(nfc->tag_in_range_event);
}

void Module::Interface::GetTagOutOfRangeEvent(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x0C, 0, 0};

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 2)};
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(nfc->tag_out_of_range_event);
}

void Module::Interface::GetTagState(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0x0D, 2, 0};
    rb.Push(RESULT_SUCCESS);
    rb.PushEnum(nfc->nfc_tag_state.load());
}

void Module::Interface::CommunicationGetStatus(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0x0F, 2, 0};
    rb.Push(RESULT_SUCCESS);
    rb.PushEnum(nfc->nfc_status);
}

void Module::Interface::OpenAppData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x13, 1, 0};
    u32 app_id{rp.Pop<u32>()};

    ResultCode result{RESULT_SUCCESS};

    nfc->mutex.lock();
    if (memcmp(&app_id, &nfc->decrypted_data[0xB6], sizeof(app_id))) {
        result = ResultCode(0xC8A17638); // app id mismatch
    } else if (!(nfc->decrypted_data[0x2C] & 0x20)) {
        result = ResultCode(0xC8A17628); // uninitialised
    }
    nfc->mutex.unlock();

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(result);
}

void Module::Interface::ReadAppData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x15, 1, 0};
    u32 size{rp.Pop<u32>()};
    ASSERT(size >= 0xD8);

    std::vector<u8> buffer(size);

    nfc->mutex.lock();
    std::memcpy(buffer.data(), &nfc->decrypted_data[0xDC], size);
    nfc->mutex.unlock();

    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 2)};
    rb.Push(RESULT_SUCCESS);
    rb.PushStaticBuffer(buffer, 0);
}

void Module::Interface::Unknown0x1A(Kernel::HLERequestContext& ctx) {
    ResultCode result{RESULT_SUCCESS};
    if (nfc->nfc_tag_state != TagState::TagInRange) {
        result = ResultCode(ErrCodes::CommandInvalidForState, ErrorModule::NFC,
                            ErrorSummary::InvalidState, ErrorLevel::Status);
    } else {
        nfc->nfc_tag_state = TagState::Unknown6;
    }

    IPC::ResponseBuilder rb{ctx, 0x1A, 1, 0};
    rb.Push(result);
}

void Module::Interface::GetIdentificationBlock(Kernel::HLERequestContext& ctx) {
    if (nfc->nfc_tag_state != TagState::TagDataLoaded && nfc->nfc_tag_state != TagState::Unknown6) {
        IPC::ResponseBuilder rb{ctx, 0x1B, 1, 0};
        rb.Push(ResultCode(ErrCodes::CommandInvalidForState, ErrorModule::NFC,
                           ErrorSummary::InvalidState, ErrorLevel::Status));
        return;
    }

    IdentificationBlockRaw identification_block_raw{};
    nfc->mutex.lock();
    std::memcpy(&identification_block_raw, &nfc->encrypted_data[0x54], 0x7);
    nfc->mutex.unlock();
    IdentificationBlockReply identification_block_reply{};
    identification_block_reply.char_id = identification_block_raw.char_id;
    identification_block_reply.char_variant = identification_block_raw.char_variant;
    identification_block_reply.series = identification_block_raw.series;
    identification_block_reply.model_number = identification_block_raw.model_number;
    identification_block_reply.figure_type = identification_block_raw.figure_type;

    IPC::ResponseBuilder rb{ctx, 0x1B, 0x1F, 0};
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw<IdentificationBlockReply>(identification_block_reply);
}

Module::Interface::Interface(std::shared_ptr<Module> nfc, const char* name)
    : ServiceFramework{name, 1}, nfc{std::move(nfc)} {}

Module::Interface::~Interface() = default;

void Module::Interface::LoadAmiibo(const std::string& path) {
    LOG_INFO(Service_NFC, "Loading amiibo {}", path);

    FileUtil::IOFile file{path, "rb"};
    nfc->mutex.lock();
    nfc->encrypted_data.fill(0);
    nfc->decrypted_data.fill(0);
    file.ReadBytes(nfc->encrypted_data.data(), nfc->encrypted_data.size());
    if (!amitool_unpack(nfc->encrypted_data.data(), nfc->encrypted_data.size(),
                        nfc->decrypted_data.data(), nfc->decrypted_data.size())) {
        LOG_ERROR(Service_NFC, "Failed to decrypt amiibo {}", path);
    }
    nfc->mutex.unlock();
    nfc->nfc_tag_state = Service::NFC::TagState::TagInRange;
    nfc->tag_in_range_event->Signal();
}

void Module::Interface::RemoveAmiibo() {
    LOG_INFO(Service_NFC, "Removing amiibo");

    nfc->mutex.lock();
    nfc->encrypted_data.fill(0);
    nfc->decrypted_data.fill(0);
    nfc->mutex.unlock();
    nfc->nfc_tag_state = Service::NFC::TagState::TagOutOfRange;
    nfc->tag_out_of_range_event->Signal();
}

Module::Module() {
    tag_in_range_event =
        Kernel::Event::Create(Kernel::ResetType::OneShot, "NFC::tag_in_range_event");
    tag_out_of_range_event =
        Kernel::Event::Create(Kernel::ResetType::OneShot, "NFC::tag_out_range_event");

    FileUtil::IOFile keys_file{
        fmt::format("{}/amiibo_keys.bin", FileUtil::GetUserPath(D_SYSDATA_IDX)), "rb"};
    if (!keys_file.IsOpen() || keys_file.GetSize() != 160) {
        LOG_ERROR(Service_NFC, "amiibo_keys.bin file not found or invalid.");
    } else {
        keys_file.ReadBytes(keys.data(), 160);
        amitool_setKeys(keys.data(), 160);
    }
}

Module::~Module() = default;

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto nfc{std::make_shared<Module>()};
    std::make_shared<NFC_M>(nfc)->InstallAsService(service_manager);
    std::make_shared<NFC_U>(nfc)->InstallAsService(service_manager);
}

} // namespace Service::NFC
