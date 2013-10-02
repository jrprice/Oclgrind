// Queue.h (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

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
    enum CommandType {EMPTY, COPY, COPY_RECT, FILL, KERNEL,
                      READ, READ_RECT, WRITE, WRITE_RECT};
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
    struct FillCommand : Command
    {
      size_t address, size;
      size_t pattern_size;
      unsigned char *pattern;
      FillCommand(const unsigned char *p, size_t sz)
      {
        type = FILL;
        pattern = new unsigned char[sz];
        pattern_size = sz;
        memcpy(pattern, p, sz);
      }
      ~FillCommand()
      {
        delete[] pattern;
      }
    };

  public:
    Queue(Device& device);
    virtual ~Queue();

    Event* enqueue(Command *command);

    void executeCopyBuffer(CopyCommand *cmd);
    void executeCopyBufferRect(CopyRectCommand *cmd);
    void executeFillBuffer(FillCommand *cmd);
    void executeKernel(KernelCommand *cmd);
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
