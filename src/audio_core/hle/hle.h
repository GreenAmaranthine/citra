// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "audio_core/audio_types.h"
#include "audio_core/sink.h"
#include "audio_core/time_stretch.h"
#include "common/common_types.h"
#include "common/ring_buffer.h"
#include "core/memory.h"

namespace Service::DSP {
class DSP_DSP;
} // namespace Service::DSP

namespace AudioCore {

class DspHle final {
public:
    DspHle();
    ~DspHle();

    /// Get the state of the DSP
    DspState GetDspState() const;

    /**
     * Reads `length` bytes from the DSP pipe identified with `pipe_number`.
     * @note Can read up to the maximum value of a u16 in bytes (65,535).
     * @note IF an error is encoutered with either an invalid `pipe_number` or `length` value, an
     * empty vector will be returned.
     * @note IF `length` is set to 0, an empty vector will be returned.
     * @note IF `length` is greater than the amount of data available, this function will only read
     * the available amount.
     * @param pipe_number a `DspPipe`
     * @param length the number of bytes to read. The max is 65,535 (max of u16).
     * @returns a vector of bytes from the specified pipe. On error, will be empty.
     */
    std::vector<u8> PipeRead(DspPipe pipe_number, u32 length);

    /**
     * How much data is left in pipe
     * @param pipe_number The Pipe ID
     * @return The amount of data remaning in the pipe. This is the maximum length PipeRead will
     * return.
     */
    std::size_t GetPipeReadableSize(DspPipe pipe_number) const;

    /**
     * Write to a DSP pipe.
     * @param pipe_number The Pipe ID
     * @param buffer The data to write to the pipe.
     */
    void PipeWrite(DspPipe pipe_number, const std::vector<u8>& buffer);

    /// Returns a reference to the array backing DSP memory
    std::array<u8, Memory::DSP_RAM_SIZE>& GetDspMemory();

    /// Sets the dsp class that we trigger interrupts for
    void SetServiceToInterrupt(std::weak_ptr<Service::DSP::DSP_DSP> dsp);

    bool IsOutputAllowed();

    /// Creates a new sink to change the audio device
    void UpdateSink();

    /// Enable/Disable audio stretching.
    void EnableStretching(bool enable);

    void OutputFrame(const StereoFrame16& frame);

private:
    void FlushResidualStretcherAudio();
    void OutputCallback(s16* buffer, std::size_t num_frames);

    std::unique_ptr<Sink> sink;
    std::atomic_bool perform_time_stretching = false;
    std::atomic_bool flushing_time_stretcher = false;
    Common::RingBuffer<s16, 0x2000, 2> fifo;
    std::array<s16, 2> last_frame{};
    TimeStretcher time_stretcher;

    struct Impl;
    friend struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace AudioCore
