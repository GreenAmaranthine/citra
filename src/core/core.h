// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include "common/common_types.h"
#include "core/hle/applets/erreula.h"
#include "core/hle/applets/swkbd.h"
#include "core/loader/loader.h"
#include "core/memory.h"
#include "core/perf_stats.h"

class EmuWindow;
class ARM_Interface;

namespace AudioCore {
class DspInterface;
}

namespace Service {
namespace SM {
class ServiceManager;
} // namespace SM
} // namespace Service

namespace Core {

struct AppletFactories {
    HLE::Applets::ErrEulaFactory erreula;
    HLE::Applets::SwkbdFactory swkbd;
};

class System {
public:
    /**
     * Gets the instance of the System singleton class.
     * @returns Reference to the instance of the System singleton class.
     */
    static System& GetInstance() {
        return s_instance;
    }

    /// Enumeration representing the return values of the System Initialize and Load process.
    enum class ResultStatus : u32 {
        Success,                    ///< Succeeded
        ErrorNotInitialized,        ///< Error trying to use core prior to initialization
        ErrorGetLoader,             ///< Error finding the correct application loader
        ErrorSystemMode,            ///< Error determining the system mode
        ErrorLoader,                ///< Error loading the specified application
        ErrorLoader_ErrorEncrypted, ///< Error loading the specified application due to encryption
        ErrorLoader_ErrorInvalidFormat, ///< Error loading the specified application due to an
                                        /// invalid format
        ErrorSystemFiles,               ///< Error in finding system files
        ErrorSharedFont,                ///< Error in finding shared font
        ErrorVideoCore,                 ///< Error in the video core
        ShutdownRequested,              ///< Emulated program requested a system shutdown 
        ErrorUnknown,                   ///< Any other error
    };

    /**
     * Run the core CPU loop
     * This function runs the core for the specified number of CPU instructions before trying to
     * update hardware. This is much faster than SingleStep (and should be equivalent), as the CPU
     * is not required to do a full dispatch with each instruction. NOTE: the number of instructions
     * requested is not guaranteed to run, as this will be interrupted preemptively if a hardware
     * update is requested (e.g. on a thread switch).
     * @param tight_loop If false, the CPU single-steps.
     * @return Result status, indicating whethor or not the operation succeeded.
     */
    ResultStatus RunLoop(bool tight_loop = true);

    /**
     * Step the CPU one instruction
     * @return Result status, indicating whethor or not the operation succeeded.
     */
    ResultStatus SingleStep();

    /// Shutdown the emulated system.
    void Shutdown();


    /// Shutdown and then load again
    void Reset();

    /// Request reset of the system
    void RequestReset() {
        reset_requested = true;
    }

    /// Request shutdown of the system
    void RequestShutdown() {
        shutdown_requested = true;
    }

    /**
     * Load an executable application.
     * @param emu_window Pointer to the host-system window used for video output and keyboard input.
     * @param filepath String path to the executable application to load on the host file system.
     * @returns ResultStatus code, indicating if the operation succeeded.
     */
    ResultStatus Load(EmuWindow* emu_window, const std::string& filepath);

    /**
     * Indicates if the emulated system is powered on (all subsystems initialized and able to run an
     * application).
     * @returns True if the emulated system is powered on, otherwise false.
     */
    bool IsPoweredOn() const {
        return cpu_core != nullptr;
    }

    /// Prepare the core emulation for a reschedule
    void PrepareReschedule();

    PerfStats::Results GetAndResetPerfStats();

    /**
     * Gets a reference to the emulated CPU.
     * @returns A reference to the emulated CPU.
     */
    ARM_Interface& CPU() {
        return *cpu_core;
    }

    /**
     * Gets a reference to the emulated DSP.
     * @returns A reference to the emulated DSP.
     */
    AudioCore::DspInterface& DSP() {
        return *dsp_core;
    }

    /**
     * Gets a reference to the service manager.
     * @returns A reference to the service manager.
     */
    Service::SM::ServiceManager& ServiceManager();

    /**
     * Gets a const reference to the service manager.
     * @returns A const reference to the service manager.
     */
    const Service::SM::ServiceManager& ServiceManager() const;

    PerfStats perf_stats;
    FrameLimiter frame_limiter;

    void SetStatus(ResultStatus new_status, const char* details = nullptr) {
        status = new_status;
        if (details) {
            status_details = details;
        }
    }

    const std::string& GetStatusDetails() const {
        return status_details;
    }

    Loader::AppLoader& GetAppLoader() const {
        return *app_loader;
    }

    AppletFactories& GetAppletFactories() const {
        return *applet_factories;
    }

private:
    /**
     * Initialize the emulated system.
     * @param emu_window Pointer to the host-system window used for video output and keyboard input.
     * @param system_mode The system mode.
     * @return ResultStatus code, indicating if the operation succeeded.
     */
    ResultStatus Init(EmuWindow* emu_window, u32 system_mode);

    /// Reschedule the core emulation
    void Reschedule();

    /// AppLoader used to load the current executing application
    std::unique_ptr<Loader::AppLoader> app_loader;

    /// Applet factories
    std::unique_ptr<AppletFactories> applet_factories;

    /// ARM11 CPU core
    std::unique_ptr<ARM_Interface> cpu_core;

    /// DSP core
    std::unique_ptr<AudioCore::DspInterface> dsp_core;

    /// When true, signals that a reschedule should happen
    bool reschedule_pending{};

    /// Service manager
    std::shared_ptr<Service::SM::ServiceManager> service_manager;

    static System s_instance;

    ResultStatus status = ResultStatus::Success;
    std::string status_details = "";
    /// Saved variables for reset
    EmuWindow* m_emu_window;
    std::string m_filepath;

    std::atomic<bool> reset_requested;
    std::atomic<bool> shutdown_requested;
};

inline ARM_Interface& CPU() {
    return System::GetInstance().CPU();
}

inline AudioCore::DspInterface& DSP() {
    return System::GetInstance().DSP();
}

} // namespace Core
