// WorkGroup.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#include <sstream>

#include "llvm/IR/Module.h"

#include "Context.h"
#include "Kernel.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace oclgrind;
using namespace std;

WorkGroup::WorkGroup(const KernelInvocation *kernelInvocation, Size3 wgid)
  : WorkGroup(kernelInvocation, wgid, kernelInvocation->getLocalSize())
{
}

WorkGroup::WorkGroup(const KernelInvocation *kernelInvocation,
                     Size3 wgid, Size3 size)
 : m_context(kernelInvocation->getContext())
{
  m_groupID   = wgid;
  m_groupSize = size;

  m_groupIndex = (m_groupID.x +
                 (m_groupID.y +
                  m_groupID.z*(kernelInvocation->getNumGroups().y) *
                  kernelInvocation->getNumGroups().x));

  // Allocate local memory
  m_localMemory = new Memory(AddrSpaceLocal, sizeof(size_t)==8 ? 16 : 8,
                             m_context);
  const Kernel *kernel = kernelInvocation->getKernel();
  for (auto value = kernel->values_begin();
            value != kernel->values_end();
            value++)
  {
    const llvm::Type *type = value->first->getType();
    if (type->isPointerTy() && type->getPointerAddressSpace() == AddrSpaceLocal)
    {
      size_t ptr = m_localMemory->allocateBuffer(value->second.size);
      m_localAddresses[value->first] = ptr;
    }
  }

  // Initialise work-items
  for (size_t k = 0; k < m_groupSize.z; k++)
  {
    for (size_t j = 0; j < m_groupSize.y; j++)
    {
      for (size_t i = 0; i < m_groupSize.x; i++)
      {
        WorkItem *workItem = new WorkItem(kernelInvocation, this,
                                          Size3(i, j, k));
        m_workItems.push_back(workItem);
        m_running.insert(workItem);
      }
    }
  }

  m_nextEvent = 1;
  m_barrier = NULL;
}

WorkGroup::~WorkGroup()
{
  // Delete work-items
  for (unsigned i = 0; i < m_workItems.size(); i++)
  {
    delete m_workItems[i];
  }

  delete m_localMemory;
}

size_t WorkGroup::async_copy(
  const WorkItem *workItem,
  const llvm::Instruction *instruction,
  AsyncCopyType type,
  size_t dest,
  size_t src,
  size_t size,
  size_t num,
  size_t srcStride,
  size_t destStride,
  size_t event)
{
  AsyncCopy copy =
  {
    instruction,
    type,
    dest,
    src,
    size,
    num,
    srcStride,
    destStride,

    event
  };

  // Check if copy has already been registered by another work-item
  list< pair<AsyncCopy,set<const WorkItem*> > >::iterator itr;
  for (itr = m_asyncCopies.begin(); itr != m_asyncCopies.end(); itr++)
  {
    if (itr->second.count(workItem))
    {
      continue;
    }

    // Check for divergence
    if ((itr->first.instruction->getDebugLoc()
         != copy.instruction->getDebugLoc()) ||
        (itr->first.type != copy.type) ||
        (itr->first.dest != copy.dest) ||
        (itr->first.src != copy.src) ||
        (itr->first.size != copy.size) ||
        (itr->first.num != copy.num) ||
        (itr->first.srcStride != copy.srcStride) ||
        (itr->first.destStride != copy.destStride))
    {
      Context::Message msg(ERROR, m_context);
      msg << "Work-group divergence detected (async copy)" << endl
          << msg.INDENT
          << "Kernel:     " << msg.CURRENT_KERNEL << endl
          << "Work-group: " << msg.CURRENT_WORK_GROUP << endl
          << endl
          << "Work-item:  " << msg.CURRENT_ENTITY << endl
          << msg.CURRENT_LOCATION << endl
          << "dest=0x" << hex << copy.dest << ", "
          << "src=0x" << hex << copy.src << endl
          << "elem_size=" << dec << copy.size << ", "
          << "num_elems=" << dec << copy.num << ", "
          << "src_stride=" << dec << copy.srcStride << ", "
          << "dest_stride=" << dec << copy.destStride << endl
          << endl
          << "Previous work-items executed:" << endl
          << itr->first.instruction << endl
          << "dest=0x" << hex << itr->first.dest << ", "
          << "src=0x" << hex << itr->first.src << endl
          << "elem_size=" << dec << itr->first.size << ", "
          << "num_elems=" << dec << itr->first.num << ", "
          << "src_stride=" << dec << itr->first.srcStride << ", "
          << "dest_stride=" << dec << itr->first.destStride << endl;
      msg.send();
    }

    itr->second.insert(workItem);
    return itr->first.event;
  }

  // Create new event if necessary
  if (copy.event == 0)
  {
    copy.event = m_nextEvent++;
  }

  // Register new copy and event
  m_asyncCopies.push_back(make_pair(copy, set<const WorkItem*>()));
  m_asyncCopies.back().second.insert(workItem);
  if (!m_events.count(event))
  {
    m_events[copy.event] = list<AsyncCopy>();
  }
  m_events[copy.event].push_back(copy);

  return copy.event;
}

void WorkGroup::clearBarrier()
{
  assert(m_barrier);

  // Check for divergence
  if (m_barrier->workItems.size() != m_workItems.size())
  {
    Context::Message msg(ERROR, m_context);
    msg << "Work-group divergence detected (barrier)" << endl
        << msg.INDENT
        << "Kernel:     " << msg.CURRENT_KERNEL << endl
        << "Work-group: " << msg.CURRENT_WORK_GROUP << endl
        << "Only " << dec << m_barrier->workItems.size() << " out of "
        << m_workItems.size() << " work-items executed barrier" << endl
        << m_barrier->instruction << endl;
    msg.send();
  }

  // Move work-items to running state
  set<WorkItem*>::iterator itr;
  for (itr = m_barrier->workItems.begin();
       itr != m_barrier->workItems.end();
       itr++)
  {
    (*itr)->clearBarrier();
    m_running.insert(*itr);
  }
  m_barrier->workItems.clear();

  // Deal with events
  while (!m_barrier->events.empty())
  {
    size_t event = m_barrier->events.front();

    // Perform copy
    list<AsyncCopy> copies = m_events[event];
    list<AsyncCopy>::iterator itr;
    for (itr = copies.begin(); itr != copies.end(); itr++)
    {
      Memory *destMem, *srcMem;
      if (itr->type == GLOBAL_TO_LOCAL)
      {
        destMem = m_localMemory;
        srcMem = m_context->getGlobalMemory();
      }
      else
      {
        destMem = m_context->getGlobalMemory();
        srcMem = m_localMemory;
      }

      size_t src = itr->src;
      size_t dest = itr->dest;
      unsigned char *buffer = new unsigned char[itr->size];
      for (unsigned i = 0; i < itr->num; i++)
      {
        srcMem->load(buffer, src, itr->size);
        destMem->store(buffer, dest, itr->size);
        src += itr->srcStride * itr->size;
        dest += itr->destStride * itr->size;
      }
      delete[] buffer;
    }
    m_events.erase(event);

    // Remove copies from list for this event
    list< pair<AsyncCopy,set<const WorkItem*> > >::iterator cItr;
    for (cItr = m_asyncCopies.begin(); cItr != m_asyncCopies.end();)
    {
      if (cItr->first.event == event)
      {
        // Check that all work-items registered the copy
        if (cItr->second.size() != m_workItems.size())
        {
          Context::Message msg(ERROR, m_context);
          msg << "Work-group divergence detected (async copy)" << endl
              << msg.INDENT
              << "Kernel:     " << msg.CURRENT_KERNEL << endl
              << "Work-group: " << msg.CURRENT_WORK_GROUP << endl
              << "Only " << dec << cItr->second.size() << " out of "
              << m_workItems.size() << " work-items executed copy" << endl
              << cItr->first.instruction << endl;
          msg.send();
        }

        cItr = m_asyncCopies.erase(cItr);
      }
      else
      {
        cItr++;
      }
    }

    m_barrier->events.remove(event);
  }

  m_context->notifyWorkGroupBarrier(this, m_barrier->fence);

  delete m_barrier;
  m_barrier = NULL;
}

const llvm::Instruction* WorkGroup::getCurrentBarrier() const
{
  return m_barrier ? m_barrier->instruction : NULL;
}

Size3 WorkGroup::getGroupID() const
{
  return m_groupID;
}

size_t WorkGroup::getGroupIndex() const
{
  return m_groupIndex;
}

Size3 WorkGroup::getGroupSize() const
{
  return m_groupSize;
}

Memory* WorkGroup::getLocalMemory() const
{
  return m_localMemory;
}

size_t WorkGroup::getLocalMemoryAddress(const llvm::Value *value) const
{
  return m_localAddresses.at(value);
}

WorkItem* WorkGroup::getNextWorkItem() const
{
  if (m_running.empty())
  {
    return NULL;
  }
  return *m_running.begin();
}

WorkItem* WorkGroup::getWorkItem(Size3 localID) const
{
  return m_workItems[localID.x +
                    (localID.y + localID.z*m_groupSize.y)*m_groupSize.x];
}

bool WorkGroup::hasBarrier() const
{
  return m_barrier;
}

void WorkGroup::notifyBarrier(WorkItem *workItem,
                              const llvm::Instruction *instruction,
                              uint64_t fence, list<size_t> events)
{
  if (!m_barrier)
  {
    // Create new barrier
    m_barrier = new Barrier;
    m_barrier->instruction = instruction;
    m_barrier->fence = fence;

    m_barrier->events = events;

    // Check for invalid events
    list<size_t>::iterator itr;
    for (itr = events.begin(); itr != events.end(); itr++)
    {
      if (!m_events.count(*itr))
      {
        m_context->logError("Invalid wait event");
      }
    }
  }
  else
  {
    // Check for divergence
    bool divergence = false;
    if (instruction->getDebugLoc() != m_barrier->instruction->getDebugLoc() ||
        fence != m_barrier->fence ||
        events.size() != m_barrier->events.size())
    {
      divergence = true;
    }

    // Check events are all the same
    int divergentEventIndex = -1;
    size_t newEvent = -1;
    size_t oldEvent = -1;
    if (!divergence)
    {
      int i = 0;
      list<size_t>::iterator cItr = events.begin();
      list<size_t>::iterator pItr = m_barrier->events.begin();
      for (; cItr != events.end(); cItr++, pItr++, i++)
      {
        if (*cItr != *pItr)
        {
          divergence = true;

          divergentEventIndex = i;
          newEvent = *cItr;
          oldEvent = *pItr;

          break;
        }
      }
    }

    if (divergence)
    {
      Context::Message msg(ERROR, m_context);
      msg << "Work-group divergence detected (barrier)" << endl
          << msg.INDENT
          << "Kernel:     " << msg.CURRENT_KERNEL << endl
          << "Work-group: " << msg.CURRENT_WORK_GROUP << endl
          << endl
          << "Work-item:  " << msg.CURRENT_ENTITY << endl
          << msg.CURRENT_LOCATION << endl
          << "fence=0x" << hex << fence << ", "
          << "num_events=" << dec << events.size() << endl;
      if (divergentEventIndex >= 0)
      {
        msg << "events[" << dec << divergentEventIndex << "]="
            << newEvent << endl;
      }
      msg << endl
          << "Previous work-items executed:" << endl
          << m_barrier->instruction << endl
          << "fence=0x" << hex << m_barrier->fence << ", "
          << "num_events=" << dec << m_barrier->events.size() << endl;
      if (divergentEventIndex >= 0)
      {
        msg << "events[" << dec << divergentEventIndex << "]="
            << oldEvent << endl;
      }
      msg.send();
    }
  }

  m_running.erase(workItem);
  m_barrier->workItems.insert(workItem);
}

void WorkGroup::notifyFinished(WorkItem *workItem)
{
  m_running.erase(workItem);

  // Check if work-group finished without waiting for all events
  if (m_running.empty() && !m_barrier && !m_events.empty())
  {
    m_context->logError("Work-item finished without waiting for events");
  }
}

bool WorkGroup::WorkItemCmp::operator()(const WorkItem *lhs,
                                        const WorkItem *rhs) const
{
  Size3 lgid = lhs->getGlobalID();
  Size3 rgid = rhs->getGlobalID();
  if (lgid.z != rgid.z)
  {
    return lgid.z < rgid.z;
  }
  if (lgid.y != rgid.y)
  {
    return lgid.y < rgid.y;
  }
  return lgid.x < rgid.x;
}
