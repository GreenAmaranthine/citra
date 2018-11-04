// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

union ResultCode;

namespace Core {
class System;
} // namespace Core

namespace Kernel::SVC {

struct MemoryInfo {
    u32 base_address;
    u32 size;
    u32 permission;
    u32 state;
};

struct PageInfo {
    u32 flags;
};

/// Values accepted by svcGetSystemInfo's type parameter.
enum class SystemInfoType {
    /**
     * Reports total used memory for all regions or a specific one, according to the extra
     * parameter. See `SystemInfoMemUsageRegion`.
     */
    REGION_MEMORY_USAGE = 0,

    /// Returns the memory usage for certain allocations done internally by the kernel.
    KERNEL_ALLOCATED_PAGES = 2,

    /**
     * This returns the total number of processes which were launched directly by the kernel.
     * For the ARM11 NATIVE_FIRM kernel, this is 5, for processes sm, fs, pm, loader, and pxi.
     */
    KERNEL_SPAWNED_PIDS = 26,
};

/**
 * Accepted by svcGetSystemInfo param with REGION_MEMORY_USAGE type. Selects a region to query
 * memory usage of.
 */
enum class SystemInfoMemUsageRegion {
    All = 0,
    Application = 1,
    System = 2,
    Base = 3,
};

enum class KernelSetStateType : u32 {
    Type0 = 0,
    Type1 = 1,
    Type2 = 2,
    Type3 = 3,
    Type4 = 4,
    Type5 = 5,
    Type6 = 6,
    Type7 = 7,
    Type8 = 8,
    Type9 = 9,
    ConfigureNew3DSCPU = 10
};

/// Map application or GSP heap memory
ResultCode ControlMemory(Core::System& system, u32* out_addr, u32 operation, u32 addr0, u32 addr1,
                         u32 size, u32 permissions);

/// Map application memory
ResultCode ControlProcessMemory(Core::System& system, Handle process, u32 addr0, u32 addr1,
                                u32 size, u32 type, u32 permissions);

/// Map application memory
ResultCode MapProcessMemory(Core::System& system, Handle process, u32 start_addr, u32 size);

/// Unmap application memory
ResultCode UnmapProcessMemory(Core::System& system, Handle process, u32 start_addr, u32 size);

void ExitProcess(Core::System& system);

/// Maps a memory block to specified address
ResultCode MapMemoryBlock(Core::System& system, Handle handle, u32 addr, u32 permissions,
                          u32 other_permissions);

ResultCode UnmapMemoryBlock(Core::System& system, Handle handle, u32 addr);

/// Connect to an OS service given the port name, returns the handle to the port to out
ResultCode ConnectToPort(Core::System& system, Handle* out_handle, VAddr port_name_address);

/// Makes a blocking IPC call to an OS service.
ResultCode SendSyncRequest(Core::System& system, Handle handle);

/// Opens a process
ResultCode OpenProcess(Core::System& system, Handle* process, u32 process_id);

/// Opens a thread
ResultCode OpenThread(Core::System& system, Handle* thread, Handle process, u32 thread_id);

/// Close a handle
ResultCode CloseHandle(Core::System& system, Handle handle);

/// Wait for a handle to synchronize, timeout after the specified nanoseconds
ResultCode WaitSynchronization1(Core::System& system, Handle handle, s64 nano_seconds);

/// Wait for the given handles to synchronize, timeout after the specified nanoseconds
ResultCode WaitSynchronizationN(Core::System& system, s32* out, VAddr handles_address,
                                s32 handle_count, bool wait_all, s64 nano_seconds);

ResultCode ReceiveIPCRequest(Core::System& system, SharedPtr<ServerSession> server_session,
                             SharedPtr<Thread> thread);

/// In a single operation, sends a IPC reply and waits for a new request.
ResultCode ReplyAndReceive(Core::System& system, s32* index, VAddr handles_address,
                           s32 handle_count, Handle reply_target);

/// Create an address arbiter (Core::System& system, to allocate access to shared resources)
ResultCode CreateAddressArbiter(Core::System& system, Handle* out_handle);

/// Arbitrate address
ResultCode ArbitrateAddress(Core::System& system, Handle handle, u32 address, u32 type, u32 value,
                            s64 nanoseconds);

void Break(Core::System&, u8 break_reason);

/// Used to output a message on a debug hardware unit - doesn'thing on a retail unit
void OutputDebugString(Core::System& system, VAddr address, int len);

/// Get resource limit
ResultCode GetResourceLimit(Core::System& system, Handle* resource_limit, Handle process_handle);

/// Get resource limit current values
ResultCode GetResourceLimitCurrentValues(Core::System& system, VAddr values,
                                         Handle resource_limit_handle, VAddr names, u32 name_count);

/// Get resource limit max values
ResultCode GetResourceLimitLimitValues(Core::System& system, VAddr values,
                                       Handle resource_limit_handle, VAddr names, u32 name_count);

/// Creates a new thread
ResultCode CreateThread(Core::System& system, Handle* out_handle, u32 priority, u32 entry_point,
                        u32 arg, u32 stack_top, s32 processor_id);

/// Called when a thread exits
void ExitThread(Core::System& system);

/// Gets the priority for the specified thread
ResultCode GetThreadPriority(Core::System& system, u32* priority, Handle handle);

/// Sets the priority for the specified thread
ResultCode SetThreadPriority(Core::System& system, Handle handle, u32 priority);

/// Create a mutex
ResultCode CreateMutex(Core::System& system, Handle* out_handle, u32 initial_locked);

/// Release a mutex
ResultCode ReleaseMutex(Core::System& system, Handle handle);

/// Get the ID of the specified process
ResultCode GetProcessId(Core::System& system, u32* process_id, Handle process_handle);

/// Get the ID of the process that owns the specified thread
ResultCode GetProcessIdOfThread(Core::System& system, u32* process_id, Handle thread_handle);

/// Get the ID for the specified thread.
ResultCode GetThreadId(Core::System& system, u32* thread_id, Handle handle);

/// Creates a semaphore
ResultCode CreateSemaphore(Core::System& system, Handle* out_handle, s32 initial_count,
                           s32 max_count);

/// Releases a certain number of slots in a semaphore
ResultCode ReleaseSemaphore(Core::System& system, s32* count, Handle handle, s32 release_count);

/// Query process memory
ResultCode QueryProcessMemory(Core::System& system, MemoryInfo* memory_info, PageInfo* page_info,
                              Handle process_handle, u32 addr);

/// Query memory
ResultCode QueryMemory(Core::System& system, MemoryInfo* memory_info, PageInfo* page_info,
                       u32 addr);

/// Create an event
ResultCode CreateEvent(Core::System& system, Handle* out_handle, u32 reset_type);

/// Duplicates a kernel handle
ResultCode DuplicateHandle(Core::System& system, Handle* out, Handle handle);

/// Signals an event
ResultCode SignalEvent(Core::System& system, Handle handle);

/// Clears an event
ResultCode ClearEvent(Core::System& system, Handle handle);

/// Creates a timer
ResultCode CreateTimer(Core::System& system, Handle* out_handle, u32 reset_type);

/// Clears a timer
ResultCode ClearTimer(Core::System& system, Handle handle);

/// Starts a timer
ResultCode SetTimer(Core::System& system, Handle handle, s64 initial, s64 interval);

/// Cancels a timer
ResultCode CancelTimer(Core::System& system, Handle handle);

/// Sleep the current thread
void SleepThread(Core::System& system, s64 nanoseconds);

/// This returns the total CPU ticks elapsed since the CPU was powered-on
s64 GetSystemTick(Core::System& system);

/// Creates a memory block at the specified address with the specified permissions and size
ResultCode CreateMemoryBlock(Core::System& system, Handle* out_handle, u32 addr, u32 size,
                             u32 my_permission, u32 other_permission);

ResultCode CreatePort(Core::System& system, Handle* server_port, Handle* client_port,
                      VAddr name_address, u32 max_sessions);

ResultCode CreateSessionToPort(Core::System& system, Handle* out_client_session,
                               Handle client_port_handle);

ResultCode CreateSession(Core::System& system, Handle* server_session, Handle* client_session);

ResultCode AcceptSession(Core::System& system, Handle* out_server_session,
                         Handle server_port_handle);

ResultCode GetSystemInfo(Core::System& system, s64* out, u32 type, s32 param);

ResultCode GetProcessInfo(Core::System& system, s64* out, Handle process_handle, u32 type);

ResultCode KernelSetState(Core::System& system, u32 type, u32 param0, u32 param1, u32 param2);

} // namespace Kernel::SVC
