// KernelInvocation.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#include <atomic>
#include <sstream>
#include <thread>

#include "Context.h"
#include "Kernel.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "Program.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace oclgrind;
using namespace std;

struct
{
  int id;
  WorkGroup* workGroup;
  WorkItem* workItem;
} static THREAD_LOCAL workerState;

static atomic<unsigned> nextGroupIndex;

KernelInvocation::KernelInvocation(const Context* context, const Kernel* kernel,
                                   unsigned int workDim, Size3 globalOffset,
                                   Size3 globalSize, Size3 localSize, bool localSizeSpecified)
    : m_context(context), m_kernel(kernel)
{
  m_workDim = workDim;
  m_globalOffset = globalOffset;
  m_globalSize = globalSize;
  m_localSize = localSize;
  m_localSizeSpecified = localSizeSpecified;

  m_numGroups.x = m_globalSize.x / m_localSize.x;
  m_numGroups.y = m_globalSize.y / m_localSize.y;
  m_numGroups.z = m_globalSize.z / m_localSize.z;
  if (!m_kernel->requiresUniformWorkGroups())
  {
    m_numGroups.x += m_globalSize.x % m_localSize.x ? 1 : 0;
    m_numGroups.y += m_globalSize.y % m_localSize.y ? 1 : 0;
    m_numGroups.z += m_globalSize.z % m_localSize.z ? 1 : 0;
  }

  // Check for user overriding number of threads
  m_numWorkers =
    getEnvInt("OCLGRIND_NUM_THREADS", thread::hardware_concurrency(), false);
  if (!m_numWorkers || !m_context->isThreadSafe())
    m_numWorkers = 1;

  // Check for quick-mode environment variable
  if (checkEnv("OCLGRIND_QUICK"))
  {
    // Only run first and last work-groups in quick-mode
    Size3 firstGroup(0, 0, 0);
    Size3 lastGroup(m_numGroups.x - 1, m_numGroups.y - 1, m_numGroups.z - 1);
    m_workGroups.push_back(firstGroup);
    if (lastGroup != firstGroup)
      m_workGroups.push_back(lastGroup);
  }
  else
  {
    for (size_t k = 0; k < m_numGroups.z; k++)
    {
      for (size_t j = 0; j < m_numGroups.y; j++)
      {
        for (size_t i = 0; i < m_numGroups.x; i++)
        {
          m_workGroups.push_back(Size3(i, j, k));
        }
      }
    }
  }
}

KernelInvocation::~KernelInvocation()
{
  // Destroy any remaining work-groups
  while (!m_runningGroups.empty())
  {
    delete m_runningGroups.front();
    m_runningGroups.pop_front();
  }
}

const Context* KernelInvocation::getContext() const
{
  return m_context;
}

const WorkGroup* KernelInvocation::getCurrentWorkGroup() const
{
  return workerState.workGroup;
}

const WorkItem* KernelInvocation::getCurrentWorkItem() const
{
  return workerState.workItem;
}

Size3 KernelInvocation::getGlobalOffset() const
{
  return m_globalOffset;
}

Size3 KernelInvocation::getGlobalSize() const
{
  return m_globalSize;
}

const Kernel* KernelInvocation::getKernel() const
{
  return m_kernel;
}

Size3 KernelInvocation::getLocalSize() const
{
  return m_localSize;
}

Size3 KernelInvocation::getNumGroups() const
{
  return m_numGroups;
}

size_t KernelInvocation::getWorkDim() const
{
  return m_workDim;
}

void KernelInvocation::run(const Context* context, Kernel* kernel,
                           unsigned int workDim, Size3 globalOffset,
                           Size3 globalSize, Size3 localSize, bool localSizeSpecified)
{
  // Create kernel invocation
  KernelInvocation* ki = new KernelInvocation(
    context, kernel, workDim, globalOffset, globalSize, localSize, localSizeSpecified);

  // Run kernel
  context->notifyKernelBegin(ki);
  ki->run();
  context->notifyKernelEnd(ki);

  delete ki;
}

void KernelInvocation::run()
{
  nextGroupIndex = 0;

  // Create worker threads
  // TODO: Run in main thread if only 1 worker
  vector<thread> threads;
  for (unsigned i = 0; i < m_numWorkers; i++)
  {
    threads.push_back(thread(&KernelInvocation::runWorker, this, i));
  }

  // Wait for workers to complete
  for (unsigned i = 0; i < m_numWorkers; i++)
  {
    threads[i].join();
  }
}

int KernelInvocation::getWorkerID() const
{
  return workerState.id;
}

void KernelInvocation::runWorker(int id)
{
  workerState.workGroup = NULL;
  workerState.workItem = NULL;
  workerState.id = id;
  try
  {
    while (true)
    {
      // Move to next work-group
      if (!m_runningGroups.empty())
      {
        // Take next work-group from running pool
        workerState.workGroup = m_runningGroups.front();
        m_runningGroups.pop_front();
      }
      else
      {
        // Take next work-group from pending pool
        unsigned index = nextGroupIndex++;
        if (index >= m_workGroups.size())
          // No more work to do
          break;

        Size3 wgid = m_workGroups[index];
        Size3 wgsize = m_localSize;

        // Handle remainder work-groups
        for (unsigned i = 0; i < 3; i++)
        {
          if (wgsize[i] * (wgid[i] + 1) > m_globalSize[i])
            wgsize[i] = m_globalSize[i] % wgsize[i];
        }

        workerState.workGroup = new WorkGroup(this, wgid, wgsize);
        m_context->notifyWorkGroupBegin(workerState.workGroup);
      }

      // Execute work-group
      workerState.workItem = workerState.workGroup->getNextWorkItem();
      while (workerState.workItem)
      {
        // Run work-item until complete or at barrier
        while (workerState.workItem->getState() == WorkItem::READY)
        {
          workerState.workItem->step();
        }

        // Move to next work-item
        workerState.workItem = workerState.workGroup->getNextWorkItem();
        if (workerState.workItem)
          continue;

        // No more work-items in READY state
        // Check if there are work-items at a barrier
        if (workerState.workGroup->hasBarrier())
        {
          // Resume execution
          workerState.workGroup->clearBarrier();
          workerState.workItem = workerState.workGroup->getNextWorkItem();
        }
      }

      // Work-group has finished
      m_context->notifyWorkGroupComplete(workerState.workGroup);
      delete workerState.workGroup;
      workerState.workGroup = NULL;
    }
  }
  catch (FatalError& err)
  {
    ostringstream info;
    info << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")" << endl
         << err.what();
    m_context->logError(info.str().c_str());

    if (workerState.workGroup)
      delete workerState.workGroup;
  }
}

bool KernelInvocation::switchWorkItem(const Size3 gid)
{
  assert(m_numWorkers == 1);

  // Compute work-group ID
  Size3 group(gid.x / m_localSize.x, gid.y / m_localSize.y,
              gid.z / m_localSize.z);

  bool found = false;
  WorkGroup* previousWorkGroup = workerState.workGroup;

  // Check if we're already running the work-group
  if (group == previousWorkGroup->getGroupID())
  {
    found = true;
  }

  // Check if work-group is in running pool
  if (!found)
  {
    std::list<WorkGroup*>::iterator rItr;
    for (rItr = m_runningGroups.begin(); rItr != m_runningGroups.end(); rItr++)
    {
      if (group == (*rItr)->getGroupID())
      {
        workerState.workGroup = *rItr;
        m_runningGroups.erase(rItr);
        found = true;
        break;
      }
    }
  }

  // Check if work-group is in pending pool
  if (!found)
  {
    std::vector<Size3>::iterator pItr;
    for (pItr = m_workGroups.begin() + nextGroupIndex;
         pItr != m_workGroups.end(); pItr++)
    {
      if (group == *pItr)
      {
        workerState.workGroup = new WorkGroup(this, group);
        m_context->notifyWorkGroupBegin(workerState.workGroup);
        found = true;

        // Re-order list of groups accordingly
        // Safe since this is not in a multi-threaded context
        m_workGroups.erase(pItr);
        m_workGroups.insert(m_workGroups.begin() + nextGroupIndex, group);
        nextGroupIndex++;

        break;
      }
    }
  }

  if (!found)
  {
    return false;
  }

  if (previousWorkGroup != workerState.workGroup)
  {
    m_runningGroups.push_back(previousWorkGroup);
  }

  // Get work-item
  Size3 lid(gid.x % m_localSize.x, gid.y % m_localSize.y,
            gid.z % m_localSize.z);
  workerState.workItem = workerState.workGroup->getWorkItem(lid);

  return true;
}

bool KernelInvocation::workGroupSizeSpecified() const
{
  return m_localSizeSpecified;
}
