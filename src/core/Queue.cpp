// Queue.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#include <algorithm>
#include <cassert>

#include "Context.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "Queue.h"

using namespace oclgrind;
using namespace std;

Queue::Queue(const Context* context, bool out_of_order)
    : m_context(context), m_out_of_order(out_of_order)
{
}

Queue::~Queue() {}

Event::Event()
{
  state = CL_QUEUED;
  queueTime = now();
  startTime = endTime = 0;
}

Event* Queue::enqueue(Command* cmd)
{
  Event* event = new Event();
  cmd->event = event;
  event->command = cmd;
  event->queue = this;
  m_queue.push_back(cmd);
  return event;
}

void Queue::executeCopyBuffer(CopyCommand* cmd)
{
  m_context->getGlobalMemory()->copy(cmd->dst, cmd->src, cmd->size);
}

void Queue::executeCopyBufferRect(CopyRectCommand* cmd)
{
  // Perform copy
  Memory* memory = m_context->getGlobalMemory();
  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      // Compute addresses
      size_t src = cmd->src + cmd->src_offset[0] + y * cmd->src_offset[1] +
                   z * cmd->src_offset[2];
      size_t dst = cmd->dst + cmd->dst_offset[0] + y * cmd->dst_offset[1] +
                   z * cmd->dst_offset[2];

      // Copy data
      memory->copy(dst, src, cmd->region[0]);
    }
  }
}

void Queue::executeFillBuffer(FillBufferCommand* cmd)
{
  Memory* memory = m_context->getGlobalMemory();
  for (unsigned i = 0; i < cmd->size / cmd->pattern_size; i++)
  {
    memory->store(cmd->pattern, cmd->address + i * cmd->pattern_size,
                  cmd->pattern_size);
  }
}

void Queue::executeFillImage(FillImageCommand* cmd)
{
  Memory* memory = m_context->getGlobalMemory();

  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      for (unsigned x = 0; x < cmd->region[0]; x++)
      {
        size_t address = cmd->base + (cmd->origin[0] + x) * cmd->pixelSize +
                         (cmd->origin[1] + y) * cmd->rowPitch +
                         (cmd->origin[2] + z) * cmd->slicePitch;
        memory->store(cmd->color, address, cmd->pixelSize);
      }
    }
  }
}

void Queue::executeKernel(KernelCommand* cmd)
{
  // Run kernel
  KernelInvocation::run(m_context, cmd->kernel, cmd->work_dim,
                        cmd->globalOffset, cmd->globalSize, cmd->localSize, cmd->localSizeSpecified);
}

void Queue::executeMap(MapCommand* cmd)
{
  m_context->notifyMemoryMap(m_context->getGlobalMemory(), cmd->address,
                             cmd->offset, cmd->size, cmd->flags);
}

void Queue::executeNativeKernel(NativeKernelCommand* cmd)
{
  // Run kernel
  cmd->func(cmd->args);
}

void Queue::executeReadBuffer(BufferCommand* cmd)
{
  m_context->getGlobalMemory()->load(cmd->ptr, cmd->address, cmd->size);
}

void Queue::executeReadBufferRect(BufferRectCommand* cmd)
{
  Memory* memory = m_context->getGlobalMemory();
  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      unsigned char* host = cmd->ptr + cmd->host_offset[0] +
                            y * cmd->host_offset[1] + z * cmd->host_offset[2];
      size_t buff = cmd->address + cmd->buffer_offset[0] +
                    y * cmd->buffer_offset[1] + z * cmd->buffer_offset[2];
      memory->load(host, buff, cmd->region[0]);
    }
  }
}

void Queue::executeUnmap(UnmapCommand* cmd)
{
  m_context->notifyMemoryUnmap(m_context->getGlobalMemory(), cmd->address,
                               cmd->ptr);
}

void Queue::executeWriteBuffer(BufferCommand* cmd)
{
  m_context->getGlobalMemory()->store(cmd->ptr, cmd->address, cmd->size);
}

void Queue::executeWriteBufferRect(BufferRectCommand* cmd)
{
  // Perform write
  Memory* memory = m_context->getGlobalMemory();
  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      const unsigned char* host = cmd->ptr + cmd->host_offset[0] +
                                  y * cmd->host_offset[1] +
                                  z * cmd->host_offset[2];
      size_t buff = cmd->address + cmd->buffer_offset[0] +
                    y * cmd->buffer_offset[1] + z * cmd->buffer_offset[2];
      memory->store(host, buff, cmd->region[0]);
    }
  }
}

bool Queue::isEmpty() const
{
  return m_queue.empty();
}

void Queue::execute(Command* command, bool flush)
{
  // Find command in queue
  auto it = std::find(m_queue.begin(), m_queue.end(), command);

  // If there is a previous (older) command in the queue AND either the queue
  // is not out of order OR needs to be flushed, then add event associated with
  // previous (older) command as a dependency
  if (it != m_queue.begin() && (!m_out_of_order || flush))
  {
    command->waitList.push_back((*std::prev(it))->event);
  }

  // Make sure all events in the wait list are complete before executing
  // current command
  while (!command->waitList.empty())
  {
    Event* evt = command->waitList.front();
    command->waitList.pop_front();

    if (evt->state < 0)
    {
      command->event->state = evt->state;
      m_queue.erase(it);
      return;
    }
    else if (evt->state != CL_COMPLETE)
    {
      if (evt->command)
      {
        // If it's not a user event, execute the associated command
        evt->queue->execute(evt->command, flush);
        command->execBefore.push_front(evt->command);
      }
      else
      {
        // If it's a user event then place it back at the of the wait list, and
        // check it later
        command->waitList.push_back(evt);
      }
    }
  }

  // Dispatch command
  command->event->startTime = now();
  command->event->state = CL_RUNNING;

  switch (command->type)
  {
  case Command::COPY:
    executeCopyBuffer((CopyCommand*)command);
    break;
  case Command::COPY_RECT:
    executeCopyBufferRect((CopyRectCommand*)command);
    break;
  case Command::EMPTY:
    break;
  case Command::FILL_BUFFER:
    executeFillBuffer((FillBufferCommand*)command);
    break;
  case Command::FILL_IMAGE:
    executeFillImage((FillImageCommand*)command);
    break;
  case Command::READ:
    executeReadBuffer((BufferCommand*)command);
    break;
  case Command::READ_RECT:
    executeReadBufferRect((BufferRectCommand*)command);
    break;
  case Command::KERNEL:
    executeKernel((KernelCommand*)command);
    break;
  case Command::MAP:
    executeMap((MapCommand*)command);
    break;
  case Command::NATIVE_KERNEL:
    executeNativeKernel((NativeKernelCommand*)command);
    break;
  case Command::UNMAP:
    executeUnmap((UnmapCommand*)command);
    break;
  case Command::WRITE:
    executeWriteBuffer((BufferCommand*)command);
    break;
  case Command::WRITE_RECT:
    executeWriteBufferRect((BufferRectCommand*)command);
    break;
  default:
    assert(false && "Unhandled command type in queue.");
  }

  command->event->endTime = now();
  command->event->state = CL_COMPLETE;

  // Remove command from its queue
  m_queue.erase(it);
}

Command* Queue::finish()
{
  if (m_queue.empty())
  {
    return NULL;
  }

  // Get most recent command in queue and execute it, triggering the execution
  // of all previous commands even if it's an out-of-order queue
  Command* cmd = m_queue.back();
  execute(cmd, true);

  return cmd;
}
