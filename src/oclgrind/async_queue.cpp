// async_queue.cpp (oclgrind)
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

#include "async_queue.h"

#include <cassert>
#include <iostream>
#include <list>
#include <map>

#include <spirsim/Kernel.h>
#include <spirsim/Queue.h>

using namespace spirsim;
using namespace std;

extern CLIicdDispatchTable *m_dispatchTable;

// Maps to keep track of retained objects
static map< Queue::Command*, list<cl_mem> > memObjectMap;
static map< Queue::Command*, cl_kernel > kernelMap;
static map< Queue::Command*, cl_event > eventMap;
static map< Queue::Command*, list<cl_event> > waitListMap;

void asyncEnqueue(cl_command_queue queue,
                  cl_command_type type,
                  Queue::Command *cmd,
                  cl_uint numEvents,
                  const cl_event *waitList,
                  cl_event *eventOut)
{
  // Add event wait list to command
  for (int i = 0; i < numEvents; i++)
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

void asyncQueueRetain(Queue::Command *cmd, cl_mem mem)
{
  // Retain object and add to map
  clRetainMemObject(mem);
  memObjectMap[cmd].push_back(mem);
}

void asyncQueueRetain(Queue::Command *cmd, cl_kernel kernel)
{
  assert(kernelMap.find(cmd) == kernelMap.end());

  // Retain kernel and add to map
  clRetainKernel(kernel);
  kernelMap[cmd] = kernel;

  // Retain memory objects arguments
  map<cl_uint,cl_mem*>::const_iterator itr;
  for (itr = kernel->memArgs.begin(); itr != kernel->memArgs.end(); itr++)
  {
    asyncQueueRetain(cmd, *itr->second);
  }
}

void asyncQueueRelease(Queue::Command *cmd)
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
  if (cmd->type == Queue::KERNEL)
  {
    assert(kernelMap.find(cmd) != kernelMap.end());
    clReleaseKernel(kernelMap[cmd]);
    kernelMap.erase(cmd);
    delete ((Queue::KernelCommand*)cmd)->kernel;
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
  clReleaseEvent(event);
}
