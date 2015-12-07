// Queue.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#include <cassert>

#include "Context.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "Queue.h"

using namespace oclgrind;
using namespace std;

Queue::Queue(const Context *context)
  : m_context(context)
{
}

Queue::~Queue()
{
}

Event::Event()
{
  state = CL_QUEUED;
  queueTime = now();
  startTime = endTime = 0;
}

Event* Queue::enqueue(Command *cmd)
{
  Event *event = new Event();
  cmd->event = event;
  m_queue.push(cmd);
  return event;
}

void Queue::executeCopyBuffer(CopyCommand *cmd)
{
  m_context->getGlobalMemory()->copy(cmd->dst, cmd->src, cmd->size);
}

void Queue::executeCopyBufferRect(CopyRectCommand *cmd)
{
  // Perform copy
  Memory *memory = m_context->getGlobalMemory();
  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      // Compute addresses
      size_t src =
        cmd->src +
        cmd->src_offset[0] +
        y * cmd->src_offset[1] +
        z * cmd->src_offset[2];
      size_t dst =
        cmd->dst +
        cmd->dst_offset[0] +
        y * cmd->dst_offset[1] +
        z * cmd->dst_offset[2];

      // Copy data
      memory->copy(dst, src, cmd->region[0]);
    }
  }
}

void Queue::executeFillBuffer(FillBufferCommand *cmd)
{
  Memory *memory = m_context->getGlobalMemory();
  for (unsigned i = 0; i < cmd->size/cmd->pattern_size; i++)
  {
    memory->store(cmd->pattern,
                  cmd->address + i*cmd->pattern_size,
                  cmd->pattern_size);
  }
}

void Queue::executeFillImage(FillImageCommand *cmd)
{
  Memory *memory = m_context->getGlobalMemory();

  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      for (unsigned x = 0; x < cmd->region[0]; x++)
      {
        size_t address = cmd->base
                       + (cmd->origin[0] + x) * cmd->pixelSize
                       + (cmd->origin[1] + y) * cmd->rowPitch
                       + (cmd->origin[2] + z) * cmd->slicePitch;
        memory->store(cmd->color, address, cmd->pixelSize);
      }
    }
  }
}

void Queue::executeKernel(KernelCommand *cmd)
{
  // Run kernel
  KernelInvocation::run(m_context,
                        cmd->kernel,
                        cmd->work_dim,
                        cmd->globalOffset,
                        cmd->globalSize,
                        cmd->localSize);
}

void Queue::executeMap(MapCommand *cmd)
{
  m_context->notifyMemoryMap(m_context->getGlobalMemory(),
                             cmd->address, cmd->offset, cmd->size, cmd->flags);
}

void Queue::executeNativeKernel(NativeKernelCommand *cmd)
{
  // Run kernel
  cmd->func(cmd->args);
}

void Queue::executeReadBuffer(BufferCommand *cmd)
{
  m_context->getGlobalMemory()->load(cmd->ptr, cmd->address, cmd->size);
}

void Queue::executeReadBufferRect(BufferRectCommand *cmd)
{
  Memory *memory = m_context->getGlobalMemory();
  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      unsigned char *host =
        cmd->ptr +
        cmd->host_offset[0] +
        y * cmd->host_offset[1] +
        z * cmd->host_offset[2];
      size_t buff =
        cmd->address +
        cmd->buffer_offset[0] +
        y * cmd->buffer_offset[1] +
        z * cmd->buffer_offset[2];
      memory->load(host, buff, cmd->region[0]);
    }
  }
}

void Queue::executeUnmap(UnmapCommand *cmd)
{
  m_context->notifyMemoryUnmap(m_context->getGlobalMemory(),
                               cmd->address, cmd->ptr);
}

void Queue::executeWriteBuffer(BufferCommand *cmd)
{
  m_context->getGlobalMemory()->store(cmd->ptr, cmd->address, cmd->size);
}

void Queue::executeWriteBufferRect(BufferRectCommand *cmd)
{
  // Perform write
  Memory *memory = m_context->getGlobalMemory();
  for (unsigned z = 0; z < cmd->region[2]; z++)
  {
    for (unsigned y = 0; y < cmd->region[1]; y++)
    {
      const unsigned char *host =
        cmd->ptr +
        cmd->host_offset[0] +
        y * cmd->host_offset[1] +
        z * cmd->host_offset[2];
      size_t buff =
        cmd->address +
        cmd->buffer_offset[0] +
        y * cmd->buffer_offset[1] +
        z * cmd->buffer_offset[2];
      memory->store(host, buff, cmd->region[0]);
    }
  }
}

bool Queue::isEmpty() const
{
  return m_queue.empty();
}

Queue::Command* Queue::update()
{
  if (m_queue.empty())
  {
    return NULL;
  }

  // Get next command
  Command *cmd = m_queue.front();

  // Check if all events in wait list have completed
  while (!cmd->waitList.empty())
  {
    if (cmd->waitList.front()->state == CL_COMPLETE)
    {
      cmd->waitList.pop_front();
    }
    else if (cmd->waitList.front()->state < 0)
    {
      cmd->event->state = cmd->waitList.front()->state;
      m_queue.pop();
      return cmd;
    }
    else
    {
      return NULL;
    }
  }

  cmd->event->startTime = now();
  cmd->event->state = CL_RUNNING;

  // Dispatch command
  switch (cmd->type)
  {
  case COPY:
    executeCopyBuffer((CopyCommand*)cmd);
    break;
  case COPY_RECT:
    executeCopyBufferRect((CopyRectCommand*)cmd);
    break;
  case EMPTY:
    break;
  case FILL_BUFFER:
    executeFillBuffer((FillBufferCommand*)cmd);
    break;
  case FILL_IMAGE:
    executeFillImage((FillImageCommand*)cmd);
    break;
  case READ:
    executeReadBuffer((BufferCommand*)cmd);
    break;
  case READ_RECT:
    executeReadBufferRect((BufferRectCommand*)cmd);
    break;
  case KERNEL:
    executeKernel((KernelCommand*)cmd);
    break;
  case MAP:
    executeMap((MapCommand*)cmd);
    break;
  case NATIVE_KERNEL:
    executeNativeKernel((NativeKernelCommand*)cmd);
    break;
  case UNMAP:
    executeUnmap((UnmapCommand*)cmd);
    break;
  case WRITE:
    executeWriteBuffer((BufferCommand*)cmd);
    break;
  case WRITE_RECT:
    executeWriteBufferRect((BufferRectCommand*)cmd);
    break;
  default:
    assert(false && "Unhandled command type in queue.");
  }

  cmd->event->endTime = now();
  cmd->event->state = CL_COMPLETE;

  // Remove command from queue and delete
  m_queue.pop();
  return cmd;
}
