// WorkGroup.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

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
  m_barrierFence = 0;
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

uint64_t WorkGroup::async_copy(AsyncCopy copy, uint64_t event)
{
  // TODO: Ensure all work-items hit same async_copy at same time?
  map< uint64_t, list<AsyncCopy> >::iterator eItr;
  for (eItr = m_pendingEvents.begin(); eItr != m_pendingEvents.end(); eItr++)
  {
    list<AsyncCopy>::iterator cItr;
    for (cItr = eItr->second.begin(); cItr != eItr->second.end(); cItr++)
    {
      if (*cItr == copy)
      {
        return eItr->first;
      }
    }
  }

  event = m_nextEvent++;
  m_pendingEvents[event] = list<AsyncCopy>();
  m_pendingEvents[event].push_back(copy);

  return event;
}

void WorkGroup::clearBarrier()
{
  // Check for divergence
  if (m_barrier.size() != m_workItems.size())
  {
    FATAL_ERROR("Barrier divergence detected");
  }

  // Move work-items to running state
  set<WorkItem*>::iterator itr;
  for (itr = m_barrier.begin(); itr != m_barrier.end(); itr++)
  {
    (*itr)->clearBarrier();
    m_running.insert(*itr);
  }
  m_barrier.clear();

  // Check if we're waiting on an event
  if (!m_waitEvents.empty())
  {
    // Perform group copy
    set<uint64_t>::iterator eItr;
    for (eItr = m_waitEvents.begin(); eItr != m_waitEvents.end(); eItr++)
    {
      list<AsyncCopy> copies = m_pendingEvents[*eItr];
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
          // TODO: Check result of load/store and produce error message
          srcMem->load(buffer, src, itr->size);
          destMem->store(buffer, dest, itr->size);
          src += itr->srcStride * itr->size;
          dest += itr->destStride * itr->size;
        }
        delete[] buffer;
      }
      m_pendingEvents.erase(*eItr);
    }
    m_waitEvents.clear();
  }

  // Apple memory fences
  if (m_barrierFence & CLK_LOCAL_MEM_FENCE)
  {
    m_localMemory->synchronize();
  }
  if (m_barrierFence & CLK_GLOBAL_MEM_FENCE)
  {
    m_device->getGlobalMemory()->synchronize(true);
  }
  m_barrierFence = 0;
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
  return !m_barrier.empty();
}

void WorkGroup::notifyBarrier(WorkItem *workItem, uint64_t fence)
{
  m_running.erase(workItem);
  m_barrier.insert(workItem);
  m_barrierFence = fence;
}

void WorkGroup::notifyFinished(WorkItem *workItem)
{
  m_running.erase(workItem);
}

void WorkGroup::wait_event(uint64_t event)
{
  // TODO: Ensure all work-items hit same wait at same time?
  assert(m_pendingEvents.count(event));
  m_waitEvents.insert(event);
}

bool WorkGroup::AsyncCopy::operator==(AsyncCopy copy) const
{
  return
    (instruction == copy.instruction) &&
    (type == copy.type) &&
    (dest == copy.dest) &&
    (src == copy.src) &&
    (size == copy.size);
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
