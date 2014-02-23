// Queue.h (Oclgrind)
// Copyright (c) 2013-2014, University of Bristol
// All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#pragma once
#include "common.h"

namespace spirsim
{
  class Device;
  class Kernel;

  struct Event
  {
    int state;
    double queueTime, startTime, endTime;
    Event();
  };

  class Queue
  {
  public:
    enum CommandType {EMPTY, COPY, COPY_RECT, FILL_BUFFER, FILL_IMAGE, KERNEL,
                      NATIVE_KERNEL, READ, READ_RECT, WRITE, WRITE_RECT};
    struct Command
    {
      CommandType type;
      std::list<Event*> waitList;
      Command()
      {
        type = EMPTY;
      }
    private:
      Event *event;
      friend class Queue;
    };
    struct BufferCommand : Command
    {
      unsigned char *ptr;
      size_t address, size;
      BufferCommand(CommandType t)
      {
        type = t;
      }
    };
    struct BufferRectCommand : Command
    {
      unsigned char *ptr;
      size_t address;
      size_t region[3];
      size_t host_offset[3];
      size_t buffer_offset[3];
      BufferRectCommand(CommandType t)
      {
        type = t;
      }
    };
    struct CopyCommand : Command
    {
      size_t src, dst, size;
      CopyCommand()
      {
        type = COPY;
      }
    };
    struct CopyRectCommand : Command
    {
      size_t src, dst;
      size_t region[3];
      size_t src_offset[3];
      size_t dst_offset[3];
      CopyRectCommand()
      {
        type = COPY_RECT;
      }
    };
    struct FillBufferCommand : Command
    {
      size_t address, size;
      size_t pattern_size;
      unsigned char *pattern;
      FillBufferCommand(const unsigned char *p, size_t sz)
      {
        type = FILL_BUFFER;
        pattern = new unsigned char[sz];
        pattern_size = sz;
        memcpy(pattern, p, sz);
      }
      ~FillBufferCommand()
      {
        delete[] pattern;
      }
    };
    struct FillImageCommand : Command
    {
      size_t base;
      size_t origin[3], region[3];
      size_t rowPitch, slicePitch;
      size_t pixelSize;
      unsigned char color[16];
      FillImageCommand(size_t b, const size_t o[3], const size_t r[3],
                       size_t rp, size_t sp,
                       size_t ps, const unsigned char *col)
      {
        type = FILL_IMAGE;
        base = b;
        memcpy(origin, o, sizeof(size_t)*3);
        memcpy(region, r, sizeof(size_t)*3);
        rowPitch = rp;
        slicePitch = sp;
        pixelSize = ps;
        memcpy(color, col, 16);
      }
    };
    struct KernelCommand : Command
    {
      Kernel *kernel;
      unsigned int work_dim;
      size_t global_offset[3];
      size_t global_size[3];
      size_t local_size[3];
      KernelCommand()
      {
        type = KERNEL;
      }
    };
    struct NativeKernelCommand : Command
    {
      void (CL_CALLBACK *func)(void *);
      void *args;
      NativeKernelCommand(void (CL_CALLBACK *f)(void *),
                          void *a, size_t sz)
      {
        type = NATIVE_KERNEL;
        func = f;
        if (a)
        {
          args = malloc(sz);
          memcpy(args, a, sz);
        }
        else
        {
          args = NULL;
        }
      }
      ~NativeKernelCommand()
      {
        if (args)
        {
          free(args);
        }
      }
    };

  public:
    Queue(Device& device);
    virtual ~Queue();

    Event* enqueue(Command *command);

    void executeCopyBuffer(CopyCommand *cmd);
    void executeCopyBufferRect(CopyRectCommand *cmd);
    void executeFillBuffer(FillBufferCommand *cmd);
    void executeFillImage(FillImageCommand *cmd);
    void executeKernel(KernelCommand *cmd);
    void executeNativeKernel(NativeKernelCommand *cmd);
    void executeReadBuffer(BufferCommand *cmd);
    void executeReadBufferRect(BufferRectCommand *cmd);
    void executeWriteBuffer(BufferCommand *cmd);
    void executeWriteBufferRect(BufferRectCommand *cmd);

    bool isEmpty() const;
    Command* update();

  private:
    Device& m_device;
    std::queue<Command*> m_queue;
  };
}
