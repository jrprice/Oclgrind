// WorkGroup.cpp (oclgrind)
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

#include "common.h"

#include "llvm/Module.h"

#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

WorkGroup::WorkGroup(const Kernel& kernel, Memory& globalMem,
                     unsigned int workDim,
                     size_t wgid_x, size_t wgid_y, size_t wgid_z,
                     const size_t globalOffset[3],
                     const size_t globalSize[3],
                     const size_t groupSize[3])
  : m_kernel(kernel), m_globalMemory(globalMem)
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

  // Allocate local memory
  m_localMemory = kernel.getLocalMemory()->clone();

  // Initialise work-items
  m_totalWorkItems = groupSize[0] * groupSize[1] * groupSize[2];
  m_workItems = new WorkItem*[m_totalWorkItems];
  for (size_t k = 0; k < groupSize[2]; k++)
  {
    for (size_t j = 0; j < groupSize[1]; j++)
    {
      for (size_t i = 0; i < groupSize[0]; i++)
      {
        WorkItem *workItem = new WorkItem(*this, kernel, globalMem, i, j, k);
        m_workItems[i + (j + k*groupSize[1])*groupSize[0]] = workItem;
      }
    }
  }

  m_nextEvent = 1;
}

WorkGroup::~WorkGroup()
{
  // Delete work-items
  for (int i = 0; i < m_totalWorkItems; i++)
  {
    delete m_workItems[i];
  }
  delete[] m_workItems;

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

void WorkGroup::dumpLocalMemory() const
{
  if (m_localMemory->getTotalAllocated() > 0)
  {
    cout << SMALL_SEPARATOR << endl << "Local Memory:";
    m_localMemory->dump();
  }
}

void WorkGroup::dumpPrivateMemory() const
{
  for (int i = 0; i < m_totalWorkItems; i++)
  {
    cout << SMALL_SEPARATOR;
    m_workItems[i]->dumpPrivateMemory();
  }
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

const size_t* WorkGroup::getGroupSize() const
{
  return m_groupSize;
}

Memory* WorkGroup::getLocalMemory() const
{
  return m_localMemory;
}

unsigned int WorkGroup::getWorkDim() const
{
  return m_workDim;
}

void WorkGroup::run()
{
  // Run until all work-items have finished
  int numFinished = 0;
  while (numFinished < m_totalWorkItems)
  {
    // Run work-items in order
    int numBarriers = 0;
    int numWaitEvents = 0;
    for (int i = 0; i < m_totalWorkItems; i++)
    {
      // Check if work-item is ready to execute
      WorkItem *workItem = m_workItems[i];
      if (workItem->getState() != WorkItem::READY)
      {
        continue;
      }

      // Run work-item until barrier or complete
      WorkItem::State state = workItem->getState();
      while (state == WorkItem::READY)
      {
        state = workItem->step();
      }

      // Update counters
      if (state == WorkItem::BARRIER)
      {
        numBarriers++;
      }
      else if (state == WorkItem::WAIT_EVENT)
      {
        numWaitEvents++;
      }
      else if (state == WorkItem::FINISHED)
      {
        numFinished++;
      }
    }

    // TODO: Handle work-items hitting different barriers
    // Check if all work-items have reached a barrier
    if (numBarriers == m_totalWorkItems)
    {
      for (int i = 0; i < m_totalWorkItems; i++)
      {
        m_workItems[i]->clearBarrier();
      }
    }
    else if (numBarriers > 0)
    {
      cerr << "Barrier divergence detected." << endl;
      return;
    }

    if (numWaitEvents == m_totalWorkItems)
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
            srcMem = &m_globalMemory;
          }
          else
          {
            destMem = &m_globalMemory;
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

      for (int i = 0; i < m_totalWorkItems; i++)
      {
        m_workItems[i]->clearBarrier();
      }
    }
    else if (numWaitEvents > 0)
    {
      cerr << "Wait for events divergence detected." << endl;
      return;
    }
  }
}

void WorkGroup::wait_event(uint64_t event)
{
  // TODO: Ensure all work-items hit same wait at same time?
  assert(m_pendingEvents.find(event) != m_pendingEvents.end());
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
