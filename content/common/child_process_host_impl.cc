// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/child_process_host_impl.h"

#include <limits>

#include "base/atomic_sequence_num.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/hash.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/safe_math.h"
#include "base/path_service.h"
#include "base/process/process_metrics.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "base/third_party/dynamic_annotations/dynamic_annotations.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/common/child_process_messages.h"
#include "content/public/common/child_process_host_delegate.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_shared_memory.h"
#include "ipc/ipc.mojom.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_channel_mojo.h"
#include "ipc/ipc_logging.h"
#include "ipc/message_filter.h"
#include "mojo/edk/embedder/embedder.h"
#include "services/service_manager/public/cpp/interface_provider.h"

#if defined(OS_LINUX)
#include "base/linux_util.h"
#elif defined(OS_WIN)
#include "content/common/font_cache_dispatcher_win.h"
#endif  // OS_LINUX

namespace {

// Global atomic to generate child process unique IDs.
base::StaticAtomicSequenceNumber g_unique_id;

}  // namespace

namespace content {

int ChildProcessHost::kInvalidUniqueID = -1;

uint64_t ChildProcessHost::kBrowserTracingProcessId =
    std::numeric_limits<uint64_t>::max();

// static
ChildProcessHost* ChildProcessHost::Create(ChildProcessHostDelegate* delegate) {
  return new ChildProcessHostImpl(delegate);
}

// static
base::FilePath ChildProcessHost::GetChildPath(int flags) {
  base::FilePath child_path;

  child_path = base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
      switches::kBrowserSubprocessPath);

#if defined(OS_LINUX)
  // Use /proc/self/exe rather than our known binary path so updates
  // can't swap out the binary from underneath us.
  // When running under Valgrind, forking /proc/self/exe ends up forking the
  // Valgrind executable, which then crashes. However, it's almost safe to
  // assume that the updates won't happen while testing with Valgrind tools.
  if (child_path.empty() && flags & CHILD_ALLOW_SELF && !RunningOnValgrind())
    child_path = base::FilePath(base::kProcSelfExe);
#endif

  // On most platforms, the child executable is the same as the current
  // executable.
  if (child_path.empty())
    PathService::Get(CHILD_PROCESS_EXE, &child_path);
  return child_path;
}

ChildProcessHostImpl::ChildProcessHostImpl(ChildProcessHostDelegate* delegate)
    : delegate_(delegate),
      opening_channel_(false) {
#if defined(OS_WIN)
  AddFilter(new FontCacheDispatcher());
#endif
}

ChildProcessHostImpl::~ChildProcessHostImpl() {
  // If a channel was never created than it wasn't registered and the filters
  // weren't notified. For the sake of symmetry don't call the matching teardown
  // functions. This is analogous to how RenderProcessHostImpl handles things.
  if (!channel_)
    return;

  for (size_t i = 0; i < filters_.size(); ++i) {
    filters_[i]->OnChannelClosing();
    filters_[i]->OnFilterRemoved();
  }
}

void ChildProcessHostImpl::AddFilter(IPC::MessageFilter* filter) {
  filters_.push_back(filter);

  if (channel_)
    filter->OnFilterAdded(channel_.get());
}

service_manager::InterfaceProvider*
ChildProcessHostImpl::GetRemoteInterfaces() {
  return delegate_->GetRemoteInterfaces();
}

void ChildProcessHostImpl::ForceShutdown() {
  Send(new ChildProcessMsg_Shutdown());
}

std::string ChildProcessHostImpl::CreateChannelMojo(
    const std::string& child_token) {
  DCHECK(channel_id_.empty());
  channel_id_ = mojo::edk::GenerateRandomToken();
  mojo::ScopedMessagePipeHandle host_handle =
      mojo::edk::CreateParentMessagePipe(channel_id_, child_token);
  channel_ = IPC::ChannelMojo::Create(std::move(host_handle),
                                      IPC::Channel::MODE_SERVER, this);
  if (!channel_ || !InitChannel())
    return std::string();

  return channel_id_;
}

void ChildProcessHostImpl::CreateChannelMojo() {
  // TODO(rockot): Remove |channel_id_| once this is the only code path by which
  // the Channel is created. For now it serves to at least mutually exclude
  // different CreateChannel* calls.
  DCHECK(channel_id_.empty());
  channel_id_ = "ChannelMojo";

  service_manager::InterfaceProvider* remote_interfaces = GetRemoteInterfaces();
  DCHECK(remote_interfaces);

  IPC::mojom::ChannelBootstrapPtr bootstrap;
  remote_interfaces->GetInterface(&bootstrap);
  channel_ = IPC::ChannelMojo::Create(bootstrap.PassInterface().PassHandle(),
                                      IPC::Channel::MODE_SERVER, this);
  DCHECK(channel_);

  bool initialized = InitChannel();
  DCHECK(initialized);
}

bool ChildProcessHostImpl::InitChannel() {
  if (!channel_->Connect())
    return false;

  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnFilterAdded(channel_.get());

  // Make sure these messages get sent first.
#if defined(IPC_MESSAGE_LOG_ENABLED)
  bool enabled = IPC::Logging::GetInstance()->Enabled();
  Send(new ChildProcessMsg_SetIPCLoggingEnabled(enabled));
#endif

  opening_channel_ = true;

  return true;
}

bool ChildProcessHostImpl::IsChannelOpening() {
  return opening_channel_;
}

bool ChildProcessHostImpl::Send(IPC::Message* message) {
  if (!channel_) {
    delete message;
    return false;
  }
  return channel_->Send(message);
}

int ChildProcessHostImpl::GenerateChildProcessUniqueId() {
  // This function must be threadsafe.
  //
  // Historically, this function returned ids started with 1, so in several
  // places in the code a value of 0 (rather than kInvalidUniqueID) was used as
  // an invalid value. So we retain those semantics.
  int id = g_unique_id.GetNext() + 1;

  CHECK_NE(0, id);
  CHECK_NE(kInvalidUniqueID, id);

  return id;
}

uint64_t ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
    int child_process_id) {
  // In single process mode, all the children are hosted in the same process,
  // therefore the generated memory dump guids should not be conditioned by the
  // child process id. The clients need not be aware of SPM and the conversion
  // takes care of the SPM special case while translating child process ids to
  // tracing process ids.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess))
    return ChildProcessHost::kBrowserTracingProcessId;

  // The hash value is incremented so that the tracing id is never equal to
  // MemoryDumpManager::kInvalidTracingProcessId.
  return static_cast<uint64_t>(
             base::Hash(reinterpret_cast<const char*>(&child_process_id),
                        sizeof(child_process_id))) +
         1;
}

bool ChildProcessHostImpl::OnMessageReceived(const IPC::Message& msg) {
#ifdef IPC_MESSAGE_LOG_ENABLED
  IPC::Logging* logger = IPC::Logging::GetInstance();
  if (msg.type() == IPC_LOGGING_ID) {
    logger->OnReceivedLoggingMessage(msg);
    return true;
  }

  if (logger->Enabled())
    logger->OnPreDispatchMessage(msg);
#endif

  bool handled = false;
  for (size_t i = 0; i < filters_.size(); ++i) {
    if (filters_[i]->OnMessageReceived(msg)) {
      handled = true;
      break;
    }
  }

  if (!handled) {
    handled = true;
    IPC_BEGIN_MESSAGE_MAP(ChildProcessHostImpl, msg)
      IPC_MESSAGE_HANDLER(ChildProcessHostMsg_ShutdownRequest,
                          OnShutdownRequest)
      // NB: The SyncAllocateGpuMemoryBuffer and DeletedGpuMemoryBuffer IPCs are
      // handled here for non-renderer child processes. For renderer processes,
      // they are handled in RenderMessageFilter.
      IPC_MESSAGE_HANDLER(ChildProcessHostMsg_SyncAllocateGpuMemoryBuffer,
                          OnAllocateGpuMemoryBuffer)
      IPC_MESSAGE_HANDLER(ChildProcessHostMsg_DeletedGpuMemoryBuffer,
                          OnDeletedGpuMemoryBuffer)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()

    if (!handled)
      handled = delegate_->OnMessageReceived(msg);
  }

#ifdef IPC_MESSAGE_LOG_ENABLED
  if (logger->Enabled())
    logger->OnPostDispatchMessage(msg);
#endif
  return handled;
}

void ChildProcessHostImpl::OnChannelConnected(int32_t peer_pid) {
  if (!peer_process_.IsValid()) {
    peer_process_ = base::Process::OpenWithExtraPrivileges(peer_pid);
    if (!peer_process_.IsValid())
       peer_process_ = delegate_->GetProcess().Duplicate();
    DCHECK(peer_process_.IsValid());
  }
  opening_channel_ = false;
  delegate_->OnChannelConnected(peer_pid);
  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnChannelConnected(peer_pid);
}

void ChildProcessHostImpl::OnChannelError() {
  opening_channel_ = false;
  delegate_->OnChannelError();

  for (size_t i = 0; i < filters_.size(); ++i)
    filters_[i]->OnChannelError();

  // This will delete host_, which will also destroy this!
  delegate_->OnChildDisconnected();
}

void ChildProcessHostImpl::OnBadMessageReceived(const IPC::Message& message) {
  delegate_->OnBadMessageReceived(message);
}

void ChildProcessHostImpl::OnShutdownRequest() {
  if (delegate_->CanShutdown())
    Send(new ChildProcessMsg_Shutdown());
}

void ChildProcessHostImpl::OnAllocateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    uint32_t width,
    uint32_t height,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gfx::GpuMemoryBufferHandle* handle) {
  // TODO(reveman): Add support for other types of GpuMemoryBuffers.

  // AllocateGpuMemoryBuffer() will check if |width| and |height| are valid
  // and handle failure in a controlled way when not. We just need to make
  // sure |usage| is supported here.
  if (gpu::GpuMemoryBufferImplSharedMemory::IsUsageSupported(usage)) {
    *handle = gpu::GpuMemoryBufferImplSharedMemory::AllocateGpuMemoryBuffer(
        id, gfx::Size(width, height), format);
  }
}

void ChildProcessHostImpl::OnDeletedGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gpu::SyncToken& sync_token) {
  // Note: Nothing to do here as ownership of shared memory backed
  // GpuMemoryBuffers is passed with IPC.
}

}  // namespace content
