// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cinttypes>
#include <map>
#include <fmt/format.h>
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu/cpu.h"
#include "core/hle/kernel/address_arbiter.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/ipc.h"
#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/kernel/semaphore.h"
#include "core/hle/kernel/server_port.h"
#include "core/hle/kernel/server_session.h"
#include "core/hle/kernel/session.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/timer.h"
#include "core/hle/kernel/vm_manager.h"
#include "core/hle/kernel/wait_object.h"
#include "core/hle/result.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/hle/service/service.h"
#include "core/settings.h"

namespace Kernel::SVC {

enum ControlMemoryOperation {
    MEMOP_FREE = 1,
    MEMOP_RESERVE = 2, // This operation seems to be unsupported in the kernel
    MEMOP_COMMIT = 3,
    MEMOP_MAP = 4,
    MEMOP_UNMAP = 5,
    MEMOP_PROTECT = 6,
    MEMOP_OPERATION_MASK = 0xFF,

    MEMOP_REGION_APP = 0x100,
    MEMOP_REGION_SYSTEM = 0x200,
    MEMOP_REGION_BASE = 0x300,
    MEMOP_REGION_MASK = 0xF00,

    MEMOP_LINEAR = 0x10000,
};

/// Map application or GSP heap memory
ResultCode ControlMemory(Core::System& system, u32* out_addr, u32 operation, u32 addr0, u32 addr1,
                         u32 size, u32 permissions) {
    LOG_DEBUG(Kernel_SVC,
              "operation=0x{:08X}, addr0=0x{:08X}, addr1=0x{:08X}, size=0x{:X}, "
              "permissions=0x{:08X}",
              operation, addr0, addr1, size, permissions);
    if ((addr0 & Memory::PAGE_MASK) != 0 || (addr1 & Memory::PAGE_MASK) != 0)
        return ERR_MISALIGNED_ADDRESS;
    if ((size & Memory::PAGE_MASK) != 0)
        return ERR_MISALIGNED_SIZE;
    u32 region{operation & MEMOP_REGION_MASK};
    operation &= ~MEMOP_REGION_MASK;
    if (region != 0)
        LOG_WARNING(Kernel_SVC, "ControlMemory with specified region not supported, region={:X}",
                    region);
    if ((permissions & (u32)MemoryPermission::ReadWrite) != permissions)
        return ERR_INVALID_COMBINATION;
    VMAPermission vma_permissions{(VMAPermission)permissions};
    auto& process{*system.Kernel().GetCurrentProcess()};
    switch (operation & MEMOP_OPERATION_MASK) {
    case MEMOP_FREE: {
        // TODO: What happens if an application tries to FREE a block of memory that has a
        // SharedMemory pointing to it?
        if (addr0 >= Memory::HEAP_VADDR && addr0 < Memory::HEAP_VADDR_END) {
            ResultCode result{process.HeapFree(addr0, size)};
            if (result.IsError())
                return result;
        } else if (addr0 >= process.GetLinearHeapBase() && addr0 < process.GetLinearHeapLimit()) {
            ResultCode result{process.LinearFree(addr0, size)};
            if (result.IsError())
                return result;
        } else
            return ERR_INVALID_ADDRESS;
        *out_addr = addr0;
        break;
    }
    case MEMOP_COMMIT: {
        if (operation & MEMOP_LINEAR) {
            CASCADE_RESULT(*out_addr, process.LinearAllocate(addr0, size, vma_permissions));
        } else {
            CASCADE_RESULT(*out_addr, process.HeapAllocate(addr0, size, vma_permissions));
        }
        break;
    }
    case MEMOP_MAP: {
        CASCADE_CODE(process.Map(addr0, addr1, size, vma_permissions));
        break;
    }
    case MEMOP_UNMAP: {
        CASCADE_CODE(process.Unmap(addr0, addr1, size, vma_permissions));
        break;
    }
    case MEMOP_PROTECT: {
        ResultCode result{process.vm_manager.ReprotectRange(addr0, size, vma_permissions)};
        if (result.IsError())
            return result;
        break;
    }
    default:
        LOG_ERROR(Kernel_SVC, "unknown operation=0x{:08X}", operation);
        return ERR_INVALID_COMBINATION;
    }
    process.vm_manager.LogLayout(Log::Level::Trace);
    return RESULT_SUCCESS;
}

/// Map application memory
ResultCode ControlProcessMemory(Core::System& system, Handle process, u32 addr0, u32 addr1,
                                u32 size, u32 type, u32 permissions) {
    LOG_INFO(Kernel_SVC, "process={}, addr0={}, addr1={}, size={}, type={}, permissions={}",
             static_cast<u32>(process), addr0, addr1, size, type, permissions);
    if ((addr0 & Memory::PAGE_MASK) != 0 || (addr1 & Memory::PAGE_MASK) != 0)
        return ERR_MISALIGNED_ADDRESS;
    if ((size & Memory::PAGE_MASK) != 0)
        return ERR_MISALIGNED_SIZE;
    if ((permissions & (u32)MemoryPermission::ReadWrite) != permissions)
        return ERR_INVALID_COMBINATION;
    VMAPermission vma_permissions{(VMAPermission)permissions};
    auto& p{*system.Kernel().GetProcessById(process)};
    switch (type & MEMOP_OPERATION_MASK) {
    case MEMOP_MAP: {
        // TODO: This is just a hack to avoid regressions until memory aliasing is implemented
        p.HeapAllocate(addr0, size, vma_permissions);
        break;
    }
    case MEMOP_UNMAP: {
        // TODO: This is just a hack to avoid regressions until memory aliasing is implemented
        ResultCode result{p.HeapFree(addr0, size)};
        if (result.IsError())
            return result;
        break;
    }
    case MEMOP_PROTECT: {
        ResultCode result{p.vm_manager.ReprotectRange(addr0, size, vma_permissions)};
        if (result.IsError())
            return result;
        break;
    }
    default:
        LOG_ERROR(Kernel_SVC, "unknown type=0x{:08X}", type);
        return ERR_INVALID_COMBINATION;
    }
    p.vm_manager.LogLayout(Log::Level::Trace);
    return RESULT_SUCCESS;
}

/// Map application memory
ResultCode MapProcessMemory(Core::System& system, Handle process, u32 start_addr, u32 size) {
    LOG_INFO(Kernel, "process={}, start_addr={}, size={}", static_cast<u32>(process), start_addr,
             size);
    if ((size & Memory::PAGE_MASK) != 0)
        return ERR_MISALIGNED_SIZE;
    auto& p{*system.Kernel().GetProcessById(process)};
    p.HeapAllocate(start_addr, size, VMAPermission::ReadWrite);
    p.vm_manager.LogLayout(Log::Level::Trace);
    return RESULT_SUCCESS;
}

/// Unmap application memory
ResultCode UnmapProcessMemory(Core::System& system, Handle process, u32 start_addr, u32 size) {
    LOG_INFO(Kernel, "process={}, start_addr={}, size={}", static_cast<u32>(process), start_addr,
             size);
    if ((size & Memory::PAGE_MASK) != 0)
        return ERR_MISALIGNED_SIZE;
    auto& p{*system.Kernel().GetProcessById(process)};
    ResultCode result{p.HeapFree(start_addr, size)};
    if (result.IsError())
        return result;
    p.vm_manager.LogLayout(Log::Level::Trace);
    return RESULT_SUCCESS;
}

void ExitProcess(Core::System& system) {
    auto& kernel{system.Kernel()};
    SharedPtr<Process> current_process{kernel.GetCurrentProcess()};
    LOG_INFO(Kernel_SVC, "Process {} exiting", current_process->process_id);
    ASSERT_MSG(current_process->status == ProcessStatus::Running, "Process has already exited");
    current_process->status = ProcessStatus::Exited;
    auto& thread_manager{kernel.GetThreadManager()};
    // Stop all the process threads that are currently waiting for objects.
    auto& thread_list{kernel.GetThreadManager().GetThreadList()};
    for (auto& thread : thread_list) {
        if (thread->owner_process != current_process)
            continue;
        if (thread == kernel.GetThreadManager().GetCurrentThread())
            continue;
        // TODO: When are the other running/ready threads terminated?
        ASSERT_MSG(thread->status == ThreadStatus::WaitSynchAny ||
                       thread->status == ThreadStatus::WaitSynchAll,
                   "Exiting processes with non-waiting threads is currently unimplemented");
        thread->Stop();
    }
    // Kill the current thread
    kernel.GetThreadManager().GetCurrentThread()->Stop();
    system.PrepareReschedule();
}

/// Maps a memory block to specified address
ResultCode MapMemoryBlock(Core::System& system, Handle handle, u32 addr, u32 permissions,
                          u32 other_permissions) {
    LOG_TRACE(Kernel_SVC,
              "handle=0x{:08X}, addr=0x{:08X}, mypermissions=0x{:08X}, otherpermission={}", handle,
              addr, permissions, other_permissions);
    auto current_process{system.Kernel().GetCurrentProcess()};
    auto shared_memory{current_process->handle_table.Get<SharedMemory>(handle)};
    if (!shared_memory) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    MemoryPermission permissions_type{static_cast<MemoryPermission>(permissions)};
    switch (permissions_type) {
    case MemoryPermission::Read:
    case MemoryPermission::Write:
    case MemoryPermission::ReadWrite:
    case MemoryPermission::Execute:
    case MemoryPermission::ReadExecute:
    case MemoryPermission::WriteExecute:
    case MemoryPermission::ReadWriteExecute:
    case MemoryPermission::DontCare:
        return shared_memory->Map(current_process.get(), addr, permissions_type,
                                  static_cast<MemoryPermission>(other_permissions));
    default:
        LOG_ERROR(Kernel_SVC, "unknown permissions=0x{:08X}", permissions);
    }
    return ERR_INVALID_COMBINATION;
}

ResultCode UnmapMemoryBlock(Core::System& system, Handle handle, u32 addr) {
    LOG_TRACE(Kernel_SVC, "memblock=0x{:08X}, addr=0x{:08X}", handle, addr);
    // TODO: Return E0A01BF5 if the address isn't in the application's heap
    SharedPtr<Process> current_process{system.Kernel().GetCurrentProcess()};
    SharedPtr<SharedMemory> shared_memory{current_process->handle_table.Get<SharedMemory>(handle)};
    if (!shared_memory) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    return shared_memory->Unmap(current_process.get(), addr);
}

/// Connect to an OS service given the port name, returns the handle to the port to out
ResultCode ConnectToPort(Core::System& system, Handle* out_handle, VAddr port_name_address) {
    if (!Memory::IsValidVirtualAddress(port_name_address))
        return ERR_NOT_FOUND;
    static constexpr std::size_t PortNameMaxLength{11};
    // Read 1 char beyond the max allowed port name to detect names that are too long.
    std::string port_name{Memory::ReadCString(port_name_address, PortNameMaxLength + 1)};
    if (port_name.size() > PortNameMaxLength)
        return ERR_PORT_NAME_TOO_LONG;
    LOG_TRACE(Kernel_SVC, "port_name={}", port_name);
    auto& kernel{system.Kernel()};
    auto it{kernel.named_ports.find(port_name)};
    if (it == kernel.named_ports.end()) {
        LOG_WARNING(Kernel_SVC, "tried to connect to unknown port: {}", port_name);
        return ERR_NOT_FOUND;
    }
    auto client_port{it->second};
    SharedPtr<ClientSession> client_session;
    CASCADE_RESULT(client_session, client_port->Connect());
    // Return the client session
    *out_handle = kernel.GetCurrentProcess()->handle_table.Create(client_session);
    return RESULT_SUCCESS;
}

/// Makes a blocking IPC call to an OS service.
ResultCode SendSyncRequest(Core::System& system, Handle handle) {
    auto& kernel{system.Kernel()};
    SharedPtr<ClientSession> session{
        kernel.GetCurrentProcess()->handle_table.Get<ClientSession>(handle)};
    if (!session) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    LOG_TRACE(Kernel_SVC, "handle=0x{:08X}({})", handle, session->GetName());
    system.PrepareReschedule();
    return session->SendSyncRequest(kernel.GetThreadManager().GetCurrentThread());
}

/// Opens a process
ResultCode OpenProcess(Core::System& system, Handle* process, u32 process_id) {
    auto ptr{system.Kernel().GetProcessById(process_id)};
    if (!ptr)
        return ERR_NOT_FOUND;
    *process = ptr->process_id;
    return RESULT_SUCCESS;
}

/// Opens a thread
ResultCode OpenThread(Core::System& system, Handle* thread, Handle process, u32 thread_id) {
    const auto& thread_list{system.Kernel().GetThreadManager().GetThreadList()};
    auto itr{
        std::find_if(thread_list.begin(), thread_list.end(), [&](const SharedPtr<Thread>& thread) {
            bool check1 = thread->thread_id == thread_id;
            bool check2 = thread->owner_process->process_id == process;
            return check1 && check2;
        })};
    if (itr == thread_list.end())
        return ERR_NOT_FOUND;
    *thread = (*itr)->thread_id;
    return RESULT_SUCCESS;
}

/// Close a handle
ResultCode CloseHandle(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "Closing handle 0x{:08X}", handle);
    return system.Kernel().GetCurrentProcess()->handle_table.Close(handle);
}

/// Wait for a handle to synchronize, timeout after the specified nanoseconds
ResultCode WaitSynchronization1(Core::System& system, Handle handle, s64 nano_seconds) {
    auto& kernel{system.Kernel()};
    auto object{kernel.GetCurrentProcess()->handle_table.Get<WaitObject>(handle)};
    Thread* thread{kernel.GetThreadManager().GetCurrentThread()};
    if (!object) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    LOG_TRACE(Kernel_SVC, "handle=0x{:08X}({}:{}), nanoseconds={}", handle, object->GetTypeName(),
              object->GetName(), nano_seconds);
    if (object->ShouldWait(thread)) {
        if (nano_seconds == 0)
            return RESULT_TIMEOUT;
        thread->wait_objects = {object};
        object->AddWaitingThread(thread);
        thread->status = ThreadStatus::WaitSynchAny;
        // Create an event to wake the thread up after the specified nanosecond delay has passed
        thread->WakeAfterDelay(nano_seconds);
        thread->wakeup_callback = [](ThreadWakeupReason reason, SharedPtr<Thread> thread,
                                     SharedPtr<WaitObject> object) {
            ASSERT(thread->status == ThreadStatus::WaitSynchAny);
            if (reason == ThreadWakeupReason::Timeout) {
                thread->SetWaitSynchronizationResult(RESULT_TIMEOUT);
                return;
            }
            ASSERT(reason == ThreadWakeupReason::Signal);
            thread->SetWaitSynchronizationResult(RESULT_SUCCESS);
            // WaitSynchronization1 doesn't have an output index like WaitSynchronizationN, so we
            // don't have to do anything else here.
        };
        system.PrepareReschedule();
        // Note: The output of this SVC will be set to RESULT_SUCCESS if the thread
        // resumes due to a signal in its wait objects.
        // Otherwise we retain the default value of timeout.
        return RESULT_TIMEOUT;
    }
    object->Acquire(thread);
    return RESULT_SUCCESS;
}

/// Wait for the given handles to synchronize, timeout after the specified nanoseconds
ResultCode WaitSynchronizationN(Core::System& system, s32* out, VAddr handles_address,
                                s32 handle_count, bool wait_all, s64 nano_seconds) {
    auto& kernel{system.Kernel()};
    Thread* thread{kernel.GetThreadManager().GetCurrentThread()};
    if (!Memory::IsValidVirtualAddress(handles_address))
        return ERR_INVALID_POINTER;
    // NOTE: on real hardware, there is no nullptr check for 'out' (tested with firmware 4.4). If
    // this happens, the running application will crash.
    ASSERT_MSG(out, "invalid output pointer specified!");
    // Check if handle_count is invalid
    if (handle_count < 0)
        return ERR_OUT_OF_RANGE;
    using ObjectPtr = SharedPtr<WaitObject>;
    std::vector<ObjectPtr> objects(handle_count);
    for (int i{}; i < handle_count; ++i) {
        Handle handle{Memory::Read32(handles_address + i * sizeof(Handle))};
        auto object{kernel.GetCurrentProcess()->handle_table.Get<WaitObject>(handle)};
        if (!object) {
            LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
            return ERR_INVALID_HANDLE;
        }
        objects[i] = object;
    }
    if (wait_all) {
        bool all_available{
            std::all_of(objects.begin(), objects.end(),
                        [thread](const ObjectPtr& object) { return !object->ShouldWait(thread); })};
        if (all_available) {
            // We can acquire all objects right now, do so.
            for (auto& object : objects)
                object->Acquire(thread);
            // Note: In this case, the `out` parameter isn't set,
            // and retains whatever value it had before.
            return RESULT_SUCCESS;
        }
        // Not all objects were available right now, prepare to suspend the thread.
        // If a timeout value of 0 was provided, just return the Timeout error code instead of
        // suspending the thread.
        if (nano_seconds == 0)
            return RESULT_TIMEOUT;
        // Put the thread to sleep
        thread->status = ThreadStatus::WaitSynchAll;
        // Add the thread to each of the objects' waiting threads.
        for (auto& object : objects)
            object->AddWaitingThread(thread);
        thread->wait_objects = std::move(objects);
        // Create an event to wake the thread up after the specified nanosecond delay has passed
        thread->WakeAfterDelay(nano_seconds);
        thread->wakeup_callback = [](ThreadWakeupReason reason, SharedPtr<Thread> thread,
                                     SharedPtr<WaitObject> object) {
            ASSERT(thread->status == ThreadStatus::WaitSynchAll);
            if (reason == ThreadWakeupReason::Timeout) {
                thread->SetWaitSynchronizationResult(RESULT_TIMEOUT);
                return;
            }
            ASSERT(reason == ThreadWakeupReason::Signal);
            thread->SetWaitSynchronizationResult(RESULT_SUCCESS);
            // The wait_all case doesn't update the output index.
        };
        system.PrepareReschedule();
        // This value gets set to -1 by default in this case, it isn't modified after this.
        *out = -1;
        // Note: The output of this SVC will be set to RESULT_SUCCESS if the thread resumes due to
        // a signal in one of its wait objects.
        return RESULT_TIMEOUT;
    } else {
        // Find the first object that is acquirable in the provided list of objects
        auto itr{std::find_if(objects.begin(), objects.end(), [thread](const ObjectPtr& object) {
            return !object->ShouldWait(thread);
        })};
        if (itr != objects.end()) {
            // We found a ready object, acquire it and set the result value
            WaitObject* object{itr->get()};
            object->Acquire(thread);
            *out = static_cast<s32>(std::distance(objects.begin(), itr));
            return RESULT_SUCCESS;
        }
        // No objects were ready to be acquired, prepare to suspend the thread.
        // If a timeout value of 0 was provided, just return the Timeout error code instead of
        // suspending the thread.
        if (nano_seconds == 0)
            return RESULT_TIMEOUT;
        // Put the thread to sleep
        thread->status = ThreadStatus::WaitSynchAny;
        // Add the thread to each of the objects' waiting threads.
        for (std::size_t i{}; i < objects.size(); ++i) {
            WaitObject* object{objects[i].get()};
            object->AddWaitingThread(thread);
        }
        thread->wait_objects = std::move(objects);
        // Note: If no handles and no timeout were given, then the thread will deadlock, this is
        // consistent with hardware behavior.
        // Create an event to wake the thread up after the specified nanosecond delay has passed
        thread->WakeAfterDelay(nano_seconds);
        thread->wakeup_callback = [](ThreadWakeupReason reason, SharedPtr<Thread> thread,
                                     SharedPtr<WaitObject> object) {
            ASSERT(thread->status == ThreadStatus::WaitSynchAny);
            if (reason == ThreadWakeupReason::Timeout) {
                thread->SetWaitSynchronizationResult(RESULT_TIMEOUT);
                return;
            }
            ASSERT(reason == ThreadWakeupReason::Signal);
            thread->SetWaitSynchronizationResult(RESULT_SUCCESS);
            thread->SetWaitSynchronizationOutput(thread->GetWaitObjectIndex(object.get()));
        };
        system.PrepareReschedule();
        // Note: The output of this SVC will be set to RESULT_SUCCESS if the thread resumes due to a
        // signal in one of its wait objects.
        // Otherwise we retain the default value of timeout, and -1 in the out parameter
        *out = -1;
        return RESULT_TIMEOUT;
    }
}

ResultCode ReceiveIPCRequest(SharedPtr<ServerSession> server_session, SharedPtr<Thread> thread) {
    if (!server_session->parent->client)
        return ERR_SESSION_CLOSED_BY_REMOTE;
    VAddr target_address{thread->GetCommandBufferAddress()};
    VAddr source_address{server_session->currently_handling->GetCommandBufferAddress()};
    ResultCode translation_result{TranslateCommandBuffer(server_session->currently_handling, thread,
                                                         source_address, target_address, false)};
    // If a translation error occurred, immediately resume the client thread.
    if (translation_result.IsError()) {
        // Set the output of SendSyncRequest in the client thread to the translation output.
        server_session->currently_handling->SetWaitSynchronizationResult(translation_result);
        server_session->currently_handling->ResumeFromWait();
        server_session->currently_handling = nullptr;
        // TODO: This path should try to wait again on the same objects.
        ASSERT_MSG(false, "ReplyAndReceive translation error behavior unimplemented");
    }
    return translation_result;
}

/// In a single operation, sends a IPC reply and waits for a new request.
ResultCode ReplyAndReceive(Core::System& system, s32* index, VAddr handles_address,
                           s32 handle_count, Handle reply_target) {
    if (!Memory::IsValidVirtualAddress(handles_address))
        return ERR_INVALID_POINTER;
    // Check if handle_count is invalid
    if (handle_count < 0)
        return ERR_OUT_OF_RANGE;
    using ObjectPtr = SharedPtr<WaitObject>;
    std::vector<ObjectPtr> objects(handle_count);
    auto& kernel{system.Kernel()};
    SharedPtr<Process> current_process{kernel.GetCurrentProcess()};
    for (int i{}; i < handle_count; ++i) {
        Handle handle{Memory::Read32(handles_address + i * sizeof(Handle))};
        auto object{current_process->handle_table.Get<WaitObject>(handle)};
        if (!object) {
            LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
            return ERR_INVALID_HANDLE;
        }
        objects[i] = object;
    }
    // We're also sending a command reply.
    // Don't send a reply if the command id in the command buffer is 0xFFFF.
    auto thread{kernel.GetThreadManager().GetCurrentThread()};
    u32 cmd_buff_header{Memory::Read32(thread->GetCommandBufferAddress())};
    IPC::Header header{cmd_buff_header};
    if (reply_target != 0 && header.command_id != 0xFFFF) {
        auto session{current_process->handle_table.Get<ServerSession>(reply_target)};
        if (!session) {
            LOG_ERROR(Kernel, "Invalid handle {:08X}", reply_target);
            return ERR_INVALID_HANDLE;
        }
        auto request_thread{std::move(session->currently_handling)};
        // Mark the request as "handled".
        session->currently_handling = nullptr;
        // Error out if there's no request thread or the session was closed.
        // TODO: Is the same error code (ClosedByRemote) returned for both of these cases?
        if (!request_thread || !session->parent->client) {
            *index = -1;
            return ERR_SESSION_CLOSED_BY_REMOTE;
        }
        auto source_address{thread->GetCommandBufferAddress()};
        auto target_address{request_thread->GetCommandBufferAddress()};
        auto translation_result{
            TranslateCommandBuffer(thread, request_thread, source_address, target_address, true)};
        // Note: The real kernel seems to always panic if the Server->Client buffer translation
        // fails for whatever reason.
        ASSERT(translation_result.IsSuccess());
        // Note: The scheduler isn't invoked here.
        request_thread->ResumeFromWait();
    }
    if (handle_count == 0) {
        *index = 0;
        // The kernel uses this value as a placeholder for the real error, and returns it when we
        // pass no handles and don't perform any reply.
        if (reply_target == 0 || header.command_id == 0xFFFF)
            return ResultCode(0xE7E3FFFF);
        return RESULT_SUCCESS;
    }
    // Find the first object that is acquirable in the provided list of objects
    auto itr{std::find_if(objects.begin(), objects.end(), [thread](const ObjectPtr& object) {
        return !object->ShouldWait(thread);
    })};
    if (itr != objects.end()) {
        // We found a ready object, acquire it and set the result value
        WaitObject* object{itr->get()};
        object->Acquire(thread);
        *index = static_cast<s32>(std::distance(objects.begin(), itr));
        if (object->GetHandleType() != HandleType::ServerSession)
            return RESULT_SUCCESS;
        auto server_session{static_cast<ServerSession*>(object)};
        return ReceiveIPCRequest(server_session, thread);
    }
    // No objects were ready to be acquired, prepare to suspend the thread.
    // Put the thread to sleep
    thread->status = ThreadStatus::WaitSynchAny;
    // Add the thread to each of the objects' waiting threads.
    for (std::size_t i{}; i < objects.size(); ++i) {
        auto object{objects[i].get()};
        object->AddWaitingThread(thread);
    }
    thread->wait_objects = std::move(objects);
    thread->wakeup_callback = [](ThreadWakeupReason reason, SharedPtr<Thread> thread,
                                 SharedPtr<WaitObject> object) {
        ASSERT(thread->status == ThreadStatus::WaitSynchAny);
        ASSERT(reason == ThreadWakeupReason::Signal);
        ResultCode result{RESULT_SUCCESS};
        if (object->GetHandleType() == HandleType::ServerSession) {
            auto server_session{DynamicObjectCast<ServerSession>(object)};
            result = ReceiveIPCRequest(server_session, thread);
        }
        thread->SetWaitSynchronizationResult(result);
        thread->SetWaitSynchronizationOutput(thread->GetWaitObjectIndex(object.get()));
    };
    system.PrepareReschedule();
    // Note: The output of this SVC will be set to RESULT_SUCCESS if the thread resumes due to a
    // signal in one of its wait objects, or to 0xC8A01836 if there was a translation error.
    // By default the index is set to -1.
    *index = -1;
    return RESULT_SUCCESS;
}

/// Create an address arbiter (to allocate access to shared resources)
ResultCode CreateAddressArbiter(Core::System& system, Handle* out_handle) {
    auto& kernel{system.Kernel()};
    auto arbiter{kernel.CreateAddressArbiter()};
    *out_handle = kernel.GetCurrentProcess()->handle_table.Create(std::move(arbiter));
    LOG_TRACE(Kernel_SVC, "returned handle=0x{:08X}", *out_handle);
    return RESULT_SUCCESS;
}

/// Arbitrate address
ResultCode ArbitrateAddress(Core::System& system, Handle handle, u32 address, u32 type, u32 value,
                            s64 nanoseconds) {
    LOG_TRACE(Kernel_SVC, "handle=0x{:08X}, address=0x{:08X}, type=0x{:08X}, value=0x{:08X}",
              handle, address, type, value);
    auto& kernel{system.Kernel()};
    auto arbiter{kernel.GetCurrentProcess()->handle_table.Get<AddressArbiter>(handle)};
    if (!arbiter) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    auto res{arbiter->ArbitrateAddress(kernel.GetThreadManager().GetCurrentThread(),
                                       static_cast<ArbitrationType>(type), address, value,
                                       nanoseconds)};
    // TODO: Identify in which specific cases this call should cause a reschedule.
    system.PrepareReschedule();
    return res;
}

void Break(Core::System&, u8 break_reason) {
    LOG_CRITICAL(Debug_Emulated, "Emulated program broke execution!");
    std::string reason_str;
    switch (break_reason) {
    case 0:
        reason_str = "PANIC";
        break;
    case 1:
        reason_str = "ASSERT";
        break;
    case 2:
        reason_str = "USER";
        break;
    default:
        reason_str = "UNKNOWN";
        break;
    }
    LOG_CRITICAL(Debug_Emulated, "Break reason: {}", reason_str);
}

/// Used to output a message on a debug hardware unit - doesn'thing on a retail unit
void OutputDebugString(Core::System&, VAddr address, int len) {
    if (len <= 0)
        return;
    std::string string(len, '\0');
    Memory::ReadBlock(address, string.data(), len);
    LOG_DEBUG(Debug_Emulated, "{}", string);
}

/// Get resource limit
ResultCode GetResourceLimit(Core::System& system, Handle* resource_limit, Handle process_handle) {
    LOG_TRACE(Kernel_SVC, "process=0x{:08X}", process_handle);
    auto current_process{system.Kernel().GetCurrentProcess()};
    auto process{current_process->handle_table.Get<Process>(process_handle)};
    if (!process) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", process_handle);
        return ERR_INVALID_HANDLE;
    }
    *resource_limit = current_process->handle_table.Create(process->resource_limit);
    return RESULT_SUCCESS;
}

/// Get resource limit current values
ResultCode GetResourceLimitCurrentValues(Core::System& system, VAddr values,
                                         Handle resource_limit_handle, VAddr names,
                                         u32 name_count) {
    LOG_TRACE(Kernel_SVC, "resource_limit={:08X}, names={:08X}, name_count={}",
              resource_limit_handle, names, name_count);
    auto resource_limit{system.Kernel().GetCurrentProcess()->handle_table.Get<ResourceLimit>(
        resource_limit_handle)};
    if (!resource_limit) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", resource_limit_handle);
        return ERR_INVALID_HANDLE;
    }
    for (unsigned int i{}; i < name_count; ++i) {
        u32 name{Memory::Read32(names + i * sizeof(u32))};
        s64 value{static_cast<s64>(resource_limit->GetCurrentResourceValue(name))};
        Memory::Write64(values + i * sizeof(u64), value);
    }
    return RESULT_SUCCESS;
}

/// Get resource limit max values
ResultCode GetResourceLimitLimitValues(Core::System& system, VAddr values,
                                       Handle resource_limit_handle, VAddr names, u32 name_count) {
    LOG_TRACE(Kernel_SVC, "resource_limit={:08X}, names={:08X}, name_count={}",
              resource_limit_handle, names, name_count);
    auto resource_limit{system.Kernel().GetCurrentProcess()->handle_table.Get<ResourceLimit>(
        resource_limit_handle)};
    if (!resource_limit) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", resource_limit_handle);
        return ERR_INVALID_HANDLE;
    }
    for (unsigned int i{}; i < name_count; ++i) {
        u32 name{Memory::Read32(names + i * sizeof(u32))};
        s64 value{static_cast<s64>(resource_limit->GetMaxResourceValue(name))};
        Memory::Write64(values + i * sizeof(u64), value);
    }
    return RESULT_SUCCESS;
}

/// Creates a new thread
ResultCode CreateThread(Core::System& system, Handle* out_handle, u32 priority, u32 entry_point,
                        u32 arg, u32 stack_top, s32 processor_id) {
    std::string name{fmt::format("thread-{:08X}", entry_point)};
    if (priority > ThreadPrioLowest)
        return ERR_OUT_OF_RANGE;
    auto& kernel{system.Kernel()};
    auto current_process{kernel.GetCurrentProcess()};
    auto& resource_limit{current_process->resource_limit};
    if (resource_limit->GetMaxResourceValue(ResourceTypes::PRIORITY) > priority)
        return ERR_NOT_AUTHORIZED;
    if (processor_id == ThreadProcessorIdDefault) {
        // Set the target CPU to the one specified in the process' exheader.
        processor_id = current_process->ideal_processor;
        ASSERT(processor_id != ThreadProcessorIdDefault);
    }
    switch (processor_id) {
    case ThreadProcessorId0:
        break;
    case ThreadProcessorIdAll:
        LOG_INFO(Kernel_SVC,
                 "Newly created thread is allowed to be run in any Core, unimplemented.");
        break;
    case ThreadProcessorId1:
        LOG_ERROR(Kernel_SVC,
                  "Newly created thread must run in the SysCore (Core1), unimplemented.");
        break;
    case ThreadProcessorId2:
        LOG_ERROR(Kernel_SVC,
                  "Newly created thread must run in the SysCore (Core2), unimplemented.");
        break;
    default:
        // TODO: Implement support for other processor IDs
        ASSERT_MSG(false, "Unsupported thread processor ID: {}", processor_id);
        break;
    }
    CASCADE_RESULT(auto thread, kernel.CreateThread(name, entry_point, priority, arg, processor_id,
                                                    stack_top, *current_process));
    thread->context->SetFpscr(FPSCR_DEFAULT_NAN | FPSCR_FLUSH_TO_ZERO |
                              FPSCR_ROUND_TOZERO); // 0x03C00000
    *out_handle = current_process->handle_table.Create(std::move(thread));
    system.PrepareReschedule();
    LOG_TRACE(Kernel_SVC,
              "entrypoint=0x{:08X} ({}), arg=0x{:08X}, stacktop=0x{:08X}, "
              "threadpriority=0x{:08X}, processorid=0x{:08X}, created handle=0x{:08X}",
              entry_point, name, arg, stack_top, priority, processor_id, *out_handle);

    return RESULT_SUCCESS;
}

/// Called when a thread exits
void ExitThread(Core::System& system) {
    LOG_TRACE(Kernel_SVC, "pc=0x{:08X}", system.CPU().GetPC());
    system.Kernel().GetThreadManager().ExitCurrentThread();
    system.PrepareReschedule();
}

/// Gets the priority for the specified thread
ResultCode GetThreadPriority(Core::System& system, u32* priority, Handle handle) {
    const auto thread{system.Kernel().GetCurrentProcess()->handle_table.Get<Thread>(handle)};
    if (!thread) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    *priority = thread->GetPriority();
    return RESULT_SUCCESS;
}

/// Sets the priority for the specified thread
ResultCode SetThreadPriority(Core::System& system, Handle handle, u32 priority) {
    if (priority > ThreadPrioLowest)
        return ERR_OUT_OF_RANGE;
    auto current_process{system.Kernel().GetCurrentProcess()};
    auto thread{current_process->handle_table.Get<Thread>(handle)};
    if (!thread) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    // Note: The kernel uses the current process's resource limit instead of
    // the one from the thread owner's resource limit.
    auto& resource_limit{current_process->resource_limit};
    if (resource_limit->GetMaxResourceValue(ResourceTypes::PRIORITY) > priority)
        return ERR_NOT_AUTHORIZED;
    thread->SetPriority(priority);
    thread->UpdatePriority();
    // Update the mutexes that this thread is waiting for
    for (auto& mutex : thread->pending_mutexes)
        mutex->UpdatePriority();
    system.PrepareReschedule();
    return RESULT_SUCCESS;
}

/// Create a mutex
ResultCode CreateMutex(Core::System& system, Handle* out_handle, u32 initial_locked) {
    auto& kernel{system.Kernel()};
    auto mutex{kernel.CreateMutex(initial_locked != 0)};
    mutex->name = fmt::format("mutex-{:08x}", system.CPU().GetReg(14));
    *out_handle = kernel.GetCurrentProcess()->handle_table.Create(std::move(mutex));
    LOG_TRACE(Kernel_SVC, "initial_locked={}, created handle: 0x{:08X}",
              initial_locked ? "true" : "false", *out_handle);
    return RESULT_SUCCESS;
}

/// Release a mutex
ResultCode ReleaseMutex(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "handle=0x{:08X}", handle);
    auto& kernel{system.Kernel()};
    auto mutex{kernel.GetCurrentProcess()->handle_table.Get<Mutex>(handle)};
    if (!mutex) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    return mutex->Release(kernel.GetThreadManager().GetCurrentThread());
}

/// Get the ID of the specified process
ResultCode GetProcessId(Core::System& system, u32* process_id, Handle process_handle) {
    LOG_TRACE(Kernel_SVC, "process=0x{:08X}", process_handle);
    const auto process{
        system.Kernel().GetCurrentProcess()->handle_table.Get<Process>(process_handle)};
    if (!process) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", process_handle);
        return ERR_INVALID_HANDLE;
    }
    *process_id = process->process_id;
    return RESULT_SUCCESS;
}

/// Get the ID of the process that owns the specified thread
ResultCode GetProcessIdOfThread(Core::System& system, u32* process_id, Handle thread_handle) {
    LOG_TRACE(Kernel_SVC, "thread=0x{:08X}", thread_handle);
    const auto thread{system.Kernel().GetCurrentProcess()->handle_table.Get<Thread>(thread_handle)};
    if (!thread) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", thread_handle);
        return ERR_INVALID_HANDLE;
    }
    const auto process{thread->owner_process};
    ASSERT_MSG(process, "Invalid parent process for thread={:#010X}", thread_handle);
    *process_id = process->process_id;
    return RESULT_SUCCESS;
}

/// Get the ID for the specified thread.
ResultCode GetThreadId(Core::System& system, u32* thread_id, Handle handle) {
    LOG_TRACE(Kernel_SVC, "thread=0x{:08X}", handle);
    const auto thread{system.Kernel().GetCurrentProcess()->handle_table.Get<Thread>(handle)};
    if (!thread) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    *thread_id = thread->GetThreadId();
    return RESULT_SUCCESS;
}

/// Creates a semaphore
ResultCode CreateSemaphore(Core::System& system, Handle* out_handle, s32 initial_count,
                           s32 max_count) {
    auto& kernel{system.Kernel()};
    CASCADE_RESULT(auto semaphore, kernel.CreateSemaphore(initial_count, max_count));
    semaphore->name = fmt::format("semaphore-{:08x}", system.CPU().GetReg(14));
    *out_handle = kernel.GetCurrentProcess()->handle_table.Create(std::move(semaphore));
    LOG_TRACE(Kernel_SVC, "initial_count={}, max_count={}, created handle: 0x{:08X}", initial_count,
              max_count, *out_handle);
    return RESULT_SUCCESS;
}

/// Releases a certain number of slots in a semaphore
ResultCode ReleaseSemaphore(Core::System& system, s32* count, Handle handle, s32 release_count) {
    LOG_TRACE(Kernel_SVC, "release_count={}, handle=0x{:08X}", release_count, handle);
    auto semaphore{system.Kernel().GetCurrentProcess()->handle_table.Get<Semaphore>(handle)};
    if (!semaphore) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    CASCADE_RESULT(*count, semaphore->Release(release_count));
    return RESULT_SUCCESS;
}

/// Query process memory
ResultCode QueryProcessMemory(Core::System& system, MemoryInfo* memory_info, PageInfo* page_info,
                              Handle process_handle, u32 addr) {
    auto process{system.Kernel().GetCurrentProcess()->handle_table.Get<Process>(process_handle)};
    if (!process) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", process_handle);
        return ERR_INVALID_HANDLE;
    }
    auto vma{process->vm_manager.FindVMA(addr)};
    if (vma == process->vm_manager.vma_map.end())
        return ERR_INVALID_ADDRESS;
    memory_info->base_address = vma->second.base;
    memory_info->permission = static_cast<u32>(vma->second.permissions);
    memory_info->size = vma->second.size;
    memory_info->state = static_cast<u32>(vma->second.meminfo_state);
    page_info->flags = 0;
    LOG_TRACE(Kernel_SVC, "process_handle=0x{:08X}, addr=0x{:08X}", process_handle, addr);
    return RESULT_SUCCESS;
}

/// Query memory
ResultCode QueryMemory(Core::System& system, MemoryInfo* memory_info, PageInfo* page_info,
                       u32 addr) {
    return QueryProcessMemory(system, memory_info, page_info, CurrentProcess, addr);
}

/// Create an event
ResultCode CreateEvent(Core::System& system, Handle* out_handle, u32 reset_type) {
    auto& kernel{system.Kernel()};
    auto evt{kernel.CreateEvent(static_cast<ResetType>(reset_type),
                                fmt::format("event-{:08x}", system.CPU().GetReg(14)))};
    *out_handle = kernel.GetCurrentProcess()->handle_table.Create(std::move(evt));
    LOG_TRACE(Kernel_SVC, "reset_type=0x{:08X}, created handle: 0x{:08X}", reset_type, *out_handle);
    return RESULT_SUCCESS;
}

/// Duplicates a kernel handle
ResultCode DuplicateHandle(Core::System& system, Handle* out, Handle handle) {
    CASCADE_RESULT(*out, system.Kernel().GetCurrentProcess()->handle_table.Duplicate(handle));
    LOG_TRACE(Kernel_SVC, "duplicated 0x{:08X} to 0x{:08X}", handle, *out);
    return RESULT_SUCCESS;
}

/// Signals an event
ResultCode SignalEvent(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "handle=0x{:08X}", handle);
    auto evt{system.Kernel().GetCurrentProcess()->handle_table.Get<Event>(handle)};
    if (!evt) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    evt->Signal();
    return RESULT_SUCCESS;
}

/// Clears an event
ResultCode ClearEvent(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "handle=0x{:08X}", handle);
    auto evt{system.Kernel().GetCurrentProcess()->handle_table.Get<Event>(handle)};
    if (!evt) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    evt->Clear();
    return RESULT_SUCCESS;
}

/// Creates a timer
ResultCode CreateTimer(Core::System& system, Handle* out_handle, u32 reset_type) {
    auto& kernel{system.Kernel()};
    auto timer{kernel.CreateTimer(static_cast<ResetType>(reset_type),
                                  fmt::format("timer-{:08x}", system.CPU().GetReg(14)))};
    *out_handle = kernel.GetCurrentProcess()->handle_table.Create(std::move(timer));
    LOG_TRACE(Kernel_SVC, "reset_type=0x{:08X}, created handle=0x{:08X}", reset_type, *out_handle);
    return RESULT_SUCCESS;
}

/// Clears a timer
ResultCode ClearTimer(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "timer=0x{:08X}", handle);
    auto timer{system.Kernel().GetCurrentProcess()->handle_table.Get<Timer>(handle)};
    if (!timer) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    timer->Clear();
    return RESULT_SUCCESS;
}

/// Starts a timer
ResultCode SetTimer(Core::System& system, Handle handle, s64 initial, s64 interval) {
    LOG_TRACE(Kernel_SVC, "timer=0x{:08X}", handle);
    if (initial < 0 || interval < 0)
        return ERR_OUT_OF_RANGE_KERNEL;
    auto timer{system.Kernel().GetCurrentProcess()->handle_table.Get<Timer>(handle)};
    if (!timer) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    timer->Set(initial, interval);
    return RESULT_SUCCESS;
}

/// Cancels a timer
ResultCode CancelTimer(Core::System& system, Handle handle) {
    LOG_TRACE(Kernel_SVC, "timer=0x{:08X}", handle);
    auto timer{system.Kernel().GetCurrentProcess()->handle_table.Get<Timer>(handle)};
    if (!timer) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", handle);
        return ERR_INVALID_HANDLE;
    }
    timer->Cancel();
    return RESULT_SUCCESS;
}

/// Sleep the current thread
void SleepThread(Core::System& system, s64 nanoseconds) {
    LOG_TRACE(Kernel_SVC, "nanoseconds={}", nanoseconds);
    auto& thread_manager{system.Kernel().GetThreadManager()};
    // Don't attempt to yield execution if there are no available threads to run,
    // this way we avoid a useless reschedule to the idle thread.
    if (nanoseconds == 0 && !thread_manager.HaveReadyThreads())
        return;
    // Sleep current thread and check for next thread to schedule
    thread_manager.WaitCurrentThread_Sleep();
    // Create an event to wake the thread up after the specified nanosecond delay has passed
    thread_manager.GetCurrentThread()->WakeAfterDelay(nanoseconds);
    system.PrepareReschedule();
}

/// This returns the total CPU ticks elapsed since the CPU was powered-on
s64 GetSystemTick(Core::System& system) {
    auto& timing{system.CoreTiming()};
    s64 result{static_cast<s64>(timing.GetTicks())};
    // Advance time to defeat dumb games (like Cubic Ninja) that busy-wait for the frame to end.
    // Measured time between two calls on a 9.2 o3DS with Ninjhax 1.1b
    timing.AddTicks(150);
    return result;
}

/// Creates a memory block at the specified address with the specified permissions and size
ResultCode CreateMemoryBlock(Core::System& system, Handle* out_handle, u32 addr, u32 size,
                             u32 my_permission, u32 other_permission) {
    if (size % Memory::PAGE_SIZE != 0)
        return ERR_MISALIGNED_SIZE;
    SharedPtr<SharedMemory> shared_memory;
    auto VerifyPermissions{[](MemoryPermission permission) {
        // SharedMemory blocks can not be created with Execute permissions
        switch (permission) {
        case MemoryPermission::None:
        case MemoryPermission::Read:
        case MemoryPermission::Write:
        case MemoryPermission::ReadWrite:
        case MemoryPermission::DontCare:
            return true;
        default:
            return false;
        }
    }};
    if (!VerifyPermissions(static_cast<MemoryPermission>(my_permission)) ||
        !VerifyPermissions(static_cast<MemoryPermission>(other_permission)))
        return ERR_INVALID_COMBINATION;
    // TODO: Processes with memory type Application aren't allowed
    // to create memory blocks with addr 0, any attempts to do so
    // should return error 0xD92007EA.
    if ((addr < Memory::PROCESS_IMAGE_VADDR || addr + size > Memory::SHARED_MEMORY_VADDR_END) &&
        addr != 0)
        return ERR_INVALID_ADDRESS;
    auto& kernel{system.Kernel()};
    auto current_process{kernel.GetCurrentProcess()};
    // When trying to create a memory block with address = 0,
    // if the process has the Shared Device Memory flag in the exheader,
    // then we have to allocate from the same region as the caller process instead of the Base
    // region.
    auto region{MemoryRegion::Base};
    if (addr == 0 && current_process->flags.shared_device_mem)
        region = current_process->flags.memory_region;
    shared_memory = kernel.CreateSharedMemory(
        current_process.get(), size, static_cast<MemoryPermission>(my_permission),
        static_cast<MemoryPermission>(other_permission), addr, region);
    *out_handle = current_process->handle_table.Create(std::move(shared_memory));
    LOG_WARNING(Kernel_SVC, "addr=0x{:08X}", addr);
    return RESULT_SUCCESS;
}

ResultCode CreatePort(Core::System& system, Handle* server_port, Handle* client_port,
                      VAddr name_address, u32 max_sessions) {
    // TODO: Implement named ports.
    ASSERT_MSG(name_address == 0, "Named ports are currently unimplemented");
    auto& kernel{system.Kernel()};
    auto current_process{kernel.GetCurrentProcess()};
    auto ports{kernel.CreatePortPair(max_sessions)};
    *client_port =
        current_process->handle_table.Create(std::move(std::get<SharedPtr<ClientPort>>(ports)));
    // Note: The real kernel also leaks the client port handle if the server port handle fails to be
    // created.
    *server_port =
        current_process->handle_table.Create(std::move(std::get<SharedPtr<ServerPort>>(ports)));
    LOG_TRACE(Kernel_SVC, "max_sessions={}", max_sessions);
    return RESULT_SUCCESS;
}

ResultCode CreateSessionToPort(Core::System& system, Handle* out_client_session,
                               Handle client_port_handle) {
    auto current_process{system.Kernel().GetCurrentProcess()};
    auto client_port{current_process->handle_table.Get<ClientPort>(client_port_handle)};
    if (!client_port) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", client_port_handle);
        return ERR_INVALID_HANDLE;
    }
    CASCADE_RESULT(auto session, client_port->Connect());
    *out_client_session = current_process->handle_table.Create(std::move(session));
    return RESULT_SUCCESS;
}

ResultCode CreateSession(Core::System& system, Handle* server_session, Handle* client_session) {
    auto& kernel{system.Kernel()};
    auto sessions{kernel.CreateSessionPair()};
    auto current_process{kernel.GetCurrentProcess()};
    auto& server{std::get<SharedPtr<ServerSession>>(sessions)};
    *server_session = current_process->handle_table.Create(std::move(server));
    auto& client{std::get<SharedPtr<ClientSession>>(sessions)};
    *client_session = current_process->handle_table.Create(std::move(client));
    LOG_TRACE(Kernel_SVC, "called");
    return RESULT_SUCCESS;
}

ResultCode AcceptSession(Core::System& system, Handle* out_server_session,
                         Handle server_port_handle) {
    auto current_process{system.Kernel().GetCurrentProcess()};
    auto server_port{current_process->handle_table.Get<ServerPort>(server_port_handle)};
    if (!server_port) {
        LOG_ERROR(Kernel, "Invalid handle {:08X}", server_port_handle);
        return ERR_INVALID_HANDLE;
    }
    CASCADE_RESULT(auto session, server_port->Accept());
    *out_server_session = current_process->handle_table.Create(std::move(session));
    return RESULT_SUCCESS;
}

ResultCode GetSystemInfo(Core::System& system, s64* out, u32 type, s32 param) {
    LOG_TRACE(Kernel_SVC, "type={}, param={}", type, param);
    auto& kernel{system.Kernel()};
    switch ((SystemInfoType)type) {
    case SystemInfoType::REGION_MEMORY_USAGE:
        switch ((SystemInfoMemUsageRegion)param) {
        case SystemInfoMemUsageRegion::All:
            *out = kernel.GetMemoryRegion(MemoryRegion::Application)->used +
                   kernel.GetMemoryRegion(MemoryRegion::System)->used +
                   kernel.GetMemoryRegion(MemoryRegion::Base)->used;
            break;
        case SystemInfoMemUsageRegion::Application:
            *out = kernel.GetMemoryRegion(MemoryRegion::Application)->used;
            break;
        case SystemInfoMemUsageRegion::System:
            *out = kernel.GetMemoryRegion(MemoryRegion::System)->used;
            break;
        case SystemInfoMemUsageRegion::Base:
            *out = kernel.GetMemoryRegion(MemoryRegion::Base)->used;
            break;
        default:
            LOG_ERROR(Kernel_SVC, "unknown GetSystemInfo 0 (param={})", param);
            *out = 0;
            break;
        }
        break;
    case SystemInfoType::KERNEL_SPAWNED_PIDS:
        *out = kernel.GetProcessListSize();
        break;
    default:
        LOG_ERROR(Kernel_SVC, "unknown GetSystemInfo {} (param={})", type, param);
        *out = 0;
        break;
    }
    // This function never returns an error, even if invalid parameters were passed.
    return RESULT_SUCCESS;
}

ResultCode GetProcessInfo(Core::System& system, s64* out, Handle process_handle, u32 type) {
    LOG_TRACE(Kernel_SVC, "process=0x{:08X}, type={}", process_handle, type);
    auto process{system.Kernel().GetCurrentProcess()->handle_table.Get<Process>(process_handle)};
    if (!process)
        return ERR_INVALID_HANDLE;
    switch (type) {
    case 0:
    case 2:
        // TODO: Type 0 returns a slightly higher number than type 2, but I'm not sure
        // what's the difference between them.
        *out = process->memory_used;
        if (*out % Memory::PAGE_SIZE != 0) {
            LOG_ERROR(Kernel_SVC, "memory size not page-aligned");
            return ERR_MISALIGNED_SIZE;
        }
        break;
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        // These are valid, but not implemented yet
        LOG_ERROR(Kernel_SVC, "unimplemented GetProcessInfo {}", type);
        break;
    case 20:
        *out = Memory::FCRAM_PADDR - process->GetLinearHeapAreaAddress();
        break;
    case 21:
    case 22:
    case 23:
        // These return a different error value than higher invalid values
        LOG_ERROR(Kernel_SVC, "unknown GetProcessInfo {}", type);
        return ERR_NOT_IMPLEMENTED;
    default:
        LOG_ERROR(Kernel_SVC, "unknown GetProcessInfo {}", type);
        return ERR_INVALID_ENUM_VALUE;
    }
    return RESULT_SUCCESS;
}

ResultCode KernelSetState(Core::System& system, u32 type, u32 param0, u32 param1, u32 param2) {
    switch (static_cast<KernelSetStateType>(type)) {
    case KernelSetStateType::Type0:
    case KernelSetStateType::Type1:
    case KernelSetStateType::Type2:
    case KernelSetStateType::Type3:
    case KernelSetStateType::Type4:
    case KernelSetStateType::Type5:
    case KernelSetStateType::Type6:
    case KernelSetStateType::Type7:
    case KernelSetStateType::Type8:
    case KernelSetStateType::Type9: {
        LOG_ERROR(Kernel_SVC, "unimplemented KernelSetState {}", static_cast<u32>(type));
        UNIMPLEMENTED();
        break;
    }
    case KernelSetStateType::ConfigureNew3DSCPU: {
        const bool n3ds{system.ServiceManager()
                            .GetService<Service::CFG::Module::Interface>("cfg:u")
                            ->GetModule()
                            ->GetNewModel()};
        bool enable_higher_core_clock{(n3ds && param0 & 0x00000001)};
        bool enable_additional_cache{(n3ds && (param0 >> 1) & 0x00000001)};
        LOG_WARNING(Kernel_SVC,
                    "unimplemented, enable_higher_core_clock={}, enable_additional_cache={}",
                    enable_higher_core_clock, enable_additional_cache);
        break;
    }
    default: {
        return ResultCode(ErrorDescription::InvalidEnumValue, ErrorModule::Kernel,
                          ErrorSummary::InvalidArgument, ErrorLevel::Permanent); // 0xF8C007F4
        break;
    }
    }
    return RESULT_SUCCESS;
}

} // namespace Kernel::SVC
