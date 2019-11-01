// async_queue.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "async_queue.h"

#include <cassert>
#include <iostream>
#include <list>
#include <map>

#include "core/Kernel.h"
#include "core/Queue.h"

using namespace oclgrind;
using namespace std;

// Maps to keep track of retained objects
static map< Command*, list<cl_mem> > memObjectMap;
static map< Command*, cl_kernel > kernelMap;
static map< Command*, cl_event > eventMap;
static map< Command*, list<cl_event> > waitListMap;

void asyncEnqueue(cl_command_queue queue,
                  cl_command_type type,
                  Command *cmd,
                  cl_uint numEvents,
                  const cl_event *waitList,
                  cl_event *eventOut)
{
  // Add event wait list to command
  for (unsigned i = 0; i < numEvents; i++)
  {
    cmd->waitList.push_back(waitList[i]->event);
    waitListMap[cmd].push_back(waitList[i]);
    clRetainEvent(waitList[i]);
  }

  // Enqueue command
  Event *event = queue->queue->enqueue(cmd);

  // Create event objects
  cl_event _event = new _cl_event;
  _event->dispatch = m_dispatchTable;
  _event->context = queue->context;
  _event->queue = queue;
  _event->type = type;
  _event->event = event;
  _event->refCount = 1;

  // Add event to map
  eventMap[cmd] = _event;

  // Pass event as output and retain (if required)
  if (eventOut)
  {
    clRetainEvent(_event);
    *eventOut = _event;
  }
}

void asyncQueueRetain(Command *cmd, cl_mem mem)
{
  // Retain object and add to map
  clRetainMemObject(mem);
  memObjectMap[cmd].push_back(mem);
}

void asyncQueueRetain(Command *cmd, cl_kernel kernel)
{
  assert(kernelMap.find(cmd) == kernelMap.end());

  // Retain kernel and add to map
  clRetainKernel(kernel);
  kernelMap[cmd] = kernel;

  // Retain memory objects arguments
  map<cl_uint,cl_mem>::const_iterator itr;
  for (itr = kernel->memArgs.begin(); itr != kernel->memArgs.end(); itr++)
  {
    asyncQueueRetain(cmd, itr->second);
  }
}

void asyncQueueRelease(Command *cmd)
{
  // Release memory objects
  if (memObjectMap.find(cmd) != memObjectMap.end())
  {
    list<cl_mem> memObjects = memObjectMap[cmd];
    while (!memObjects.empty())
    {
      clReleaseMemObject(memObjects.front());
      memObjects.pop_front();
    }
    memObjectMap.erase(cmd);
  }

  // Release kernel
  if (cmd->type == Command::KERNEL)
  {
    assert(kernelMap.find(cmd) != kernelMap.end());
    clReleaseKernel(kernelMap[cmd]);
    kernelMap.erase(cmd);
    delete ((KernelCommand*)cmd)->kernel;
  }

  // Remove event from map
  cl_event event = eventMap[cmd];
  eventMap.erase(cmd);

  // Perform callbacks
  list< pair<void (CL_CALLBACK *)(cl_event, cl_int, void *),
             void*> >::iterator callItr;
  for (callItr = event->callbacks.begin();
       callItr != event->callbacks.end();
       callItr++)
  {
    callItr->first(event, event->event->state, callItr->second);
  }

  // Release events
  list<cl_event>::iterator waitItr;
  for (waitItr = waitListMap[cmd].begin();
       waitItr != waitListMap[cmd].end();
       waitItr++)
  {
    clReleaseEvent(*waitItr);
  }
  waitListMap.erase(cmd);
  clReleaseEvent(event);
}
