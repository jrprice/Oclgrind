// WorkGroup.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <sstream>

#include "llvm/Module.h"

#include "Device.h"
#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

WorkGroup::WorkGroup(Device *device, const Kernel& kernel, Memory& globalMem,
                     unsigned int workDim,
                     size_t wgid_x, size_t wgid_y, size_t wgid_z,
                     const size_t globalOffset[3],
                     const size_t globalSize[3],
                     const size_t groupSize[3])
  : m_device(device), m_kernel(kernel)
{
  m_workDim = workDim;
  m_groupID[0] = wgid_x;
  m_groupID[1] = wgid_y;
  m_groupID[2] = wgid_z;
  m_globalOffset[0] = globalOffset[0];
  m_globalOffset[1] = globalOffset[1];
  m_globalOffset[2] = globalOffset[2];
  m_globalSize[0] = globalSize[0];
  m_globalSize[1] = globalSize[1];
  m_globalSize[2] = globalSize[2];
  m_groupSize[0] = groupSize[0];
  m_groupSize[1] = groupSize[1];
  m_groupSize[2] = groupSize[2];

  m_groupIndex = (m_groupID[0] +
                 (m_groupID[1] +
                  m_groupID[2]*(globalSize[1]/groupSize[1])) *
                  (globalSize[0]/groupSize[0]));

  // Allocate local memory
  m_localMemory = kernel.getLocalMemory()->clone();
  m_localMemory->setDevice(device);

  // Initialise work-items
  for (size_t k = 0; k < groupSize[2]; k++)
  {
    for (size_t j = 0; j < groupSize[1]; j++)
    {
      for (size_t i = 0; i < groupSize[0]; i++)
      {
        WorkItem *workItem = new WorkItem(m_device, *this, kernel, i, j, k);
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
  for (int i = 0; i < m_workItems.size(); i++)
  {
    delete m_workItems[i];
  }

  delete m_localMemory;
}

uint64_t WorkGroup::async_copy(
  const WorkItem *workItem,
  const llvm::Instruction *instruction,
  AsyncCopyType type,
  size_t dest,
  size_t src,
  size_t size,
  size_t num,
  size_t srcStride,
  size_t destStride,
  uint64_t event)
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
    if ((itr->first.instruction != copy.instruction) ||
        (itr->first.type != copy.type) ||
        (itr->first.dest != copy.dest) ||
        (itr->first.src != copy.src) ||
        (itr->first.size != copy.size) ||
        (itr->first.num != copy.num) ||
        (itr->first.srcStride != copy.srcStride) ||
        (itr->first.destStride != copy.destStride))
    {
      ostringstream current, previous;
      current << "dest=0x" << hex << copy.dest << ", ";
      current << "src=0x" << hex << copy.src << endl << "\t";
      current << "elem_size=" << dec << copy.size << ", ";
      current << "num_elems=" << dec << copy.num << ", ";
      current << "src_stride=" << dec << copy.srcStride << ", ";
      current << "dest_stride=" << dec << copy.destStride;
      previous << "dest=0x" << hex << itr->first.dest << ", ";
      previous << "src=0x" << hex << itr->first.src << endl << "\t";
      previous << "elem_size=" << dec << itr->first.size << ", ";
      previous << "num_elems=" << dec << itr->first.num << ", ";
      previous << "src_stride=" << dec << itr->first.srcStride << ", ";
      previous << "dest_stride=" << dec << itr->first.destStride;
      m_device->notifyDivergence(copy.instruction, "async copy",
                                 current.str(), previous.str());
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
    ostringstream info;
            info << "Only " << dec << m_barrier->workItems.size() << " out of "
                 << m_workItems.size() << " work-items executed barrier";
    m_device->notifyDivergence(m_barrier->instruction, "barrier",
                               info.str());
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

  // Check if we're waiting on an event
  if (!m_waitEvents.empty())
  {
    // Perform group copy
    set<uint64_t>::iterator eItr;
    for (eItr = m_waitEvents.begin(); eItr != m_waitEvents.end(); eItr++)
    {
      list<AsyncCopy> copies = m_events[*eItr];
      list<AsyncCopy>::iterator itr;
      for (itr = copies.begin(); itr != copies.end(); itr++)
      {
        Memory *destMem, *srcMem;
        if (itr->type == GLOBAL_TO_LOCAL)
        {
          destMem = m_localMemory;
          srcMem = m_device->getGlobalMemory();
        }
        else
        {
          destMem = m_device->getGlobalMemory();
          srcMem = m_localMemory;
        }

        size_t src = itr->src;
        size_t dest = itr->dest;
        unsigned char *buffer = new unsigned char[itr->size];
        for (int i = 0; i < itr->num; i++)
        {
          srcMem->load(buffer, src, itr->size);
          destMem->store(buffer, dest, itr->size);
          src += itr->srcStride * itr->size;
          dest += itr->destStride * itr->size;
        }
        delete[] buffer;
      }
      m_events.erase(*eItr);

      // Remove copies from list for this event
      list< pair<AsyncCopy,set<const WorkItem*> > >::iterator cItr;
      for (cItr = m_asyncCopies.begin(); cItr != m_asyncCopies.end();)
      {
        if (cItr->first.event == *eItr)
        {
          // Check that all work-items registered the copy
          if (cItr->second.size() != m_workItems.size())
          {
            ostringstream info;
            info << "Only " << dec << cItr->second.size() << " out of "
                 << m_workItems.size() << " work-items executed copy";
            m_device->notifyDivergence(cItr->first.instruction, "async_copy",
                                       info.str());
          }

          cItr = m_asyncCopies.erase(cItr);
        }
        else
        {
          cItr++;
        }
      }
    }
    m_waitEvents.clear();
  }

  // Apple memory fences
  if (m_barrier->fence & CLK_LOCAL_MEM_FENCE)
  {
    m_localMemory->synchronize();
  }
  if (m_barrier->fence & CLK_GLOBAL_MEM_FENCE)
  {
    m_device->getGlobalMemory()->synchronize(true);
  }
  delete m_barrier;
  m_barrier = NULL;
}

const size_t* WorkGroup::getGlobalOffset() const
{
  return m_globalOffset;
}

const size_t* WorkGroup::getGlobalSize() const
{
  return m_globalSize;
}

const size_t* WorkGroup::getGroupID() const
{
  return m_groupID;
}

const size_t WorkGroup::getGroupIndex() const
{
  return m_groupIndex;
}

const size_t* WorkGroup::getGroupSize() const
{
  return m_groupSize;
}

Memory* WorkGroup::getLocalMemory() const
{
  return m_localMemory;
}

WorkItem* WorkGroup::getNextWorkItem() const
{
  if (m_running.empty())
  {
    return NULL;
  }
  return *m_running.begin();
}

unsigned int WorkGroup::getWorkDim() const
{
  return m_workDim;
}

WorkItem* WorkGroup::getWorkItem(size_t localID[3]) const
{
  return m_workItems[localID[0] +
                    (localID[1] + localID[2]*m_groupSize[1])*m_groupSize[0]];
}

bool WorkGroup::hasBarrier() const
{
  return m_barrier;
}

void WorkGroup::notifyBarrier(WorkItem *workItem,
                              const llvm::Instruction *instruction,
                              uint64_t fence)
{
  if (!m_barrier)
  {
    // Create new barrier
    m_barrier = new Barrier;
    m_barrier->instruction = instruction;
    m_barrier->fence = fence;
  }
  else
  {
    // Check for divergence
    if (instruction != m_barrier->instruction ||
        fence != m_barrier->fence)
    {
      ostringstream current, previous;
      current << "fence=0x" << hex << fence;
      previous << "fence=0x" << hex << m_barrier->fence;
      m_device->notifyDivergence(m_barrier->instruction, "barrier",
                                 current.str(), previous.str());
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
    m_device->notifyError("Work-group finished without waiting for events");
  }
}

void WorkGroup::wait_event(uint64_t event)
{
  // Ensure event is valid
  if (!m_events.count(event))
  {
    m_device->notifyError("Invalid wait event");
  }

  // TODO: Ensure all work-items hit same wait at same time?
  m_waitEvents.insert(event);
}

bool WorkGroup::WorkItemCmp::operator()(const WorkItem *lhs,
                                        const WorkItem *rhs) const
{
  const size_t *lgid = lhs->getGlobalID();
  const size_t *rgid = rhs->getGlobalID();
  if (lgid[2] != rgid[2])
  {
    return lgid[2] < rgid[2];
  }
  if (lgid[1] != rgid[1])
  {
    return lgid[1] < rgid[1];
  }
  return lgid[0] < rgid[0];
}
