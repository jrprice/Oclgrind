// KernelInvocation.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <sstream>

#include "Context.h"
#include "Kernel.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace oclgrind;
using namespace std;

KernelInvocation::KernelInvocation(const Context *context, const Kernel *kernel,
                                   unsigned int workDim,
                                   Size3 globalOffset,
                                   Size3 globalSize,
                                   Size3 localSize)
  : m_context(context), m_kernel(kernel)
{
  m_globalOffset = globalOffset;
  m_globalSize   = globalSize;
  m_localSize    = localSize;

  // Create pool of pending work-groups
  m_numGroups.x = m_globalSize.x/m_localSize.x;
  m_numGroups.y = m_globalSize.y/m_localSize.y;
  m_numGroups.z = m_globalSize.z/m_localSize.z;

  // Check for quick-mode environment variable
  const char *quick = getenv("OCLGRIND_QUICK");
  if (quick && strcmp(quick, "1") == 0)
  {
    // Only run first and last work-groups in quick-mode
    Size3 firstGroup(0, 0, 0);
    Size3 lastGroup(m_numGroups.x-1, m_numGroups.y-1, m_numGroups.z-1);
    m_pendingGroups.push_back(firstGroup);
    m_pendingGroups.push_back(lastGroup);
  }
  else
  {
    for (size_t k = 0; k < m_numGroups.z; k++)
    {
      for (size_t j = 0; j < m_numGroups.y; j++)
      {
        for (size_t i = 0; i < m_numGroups.x; i++)
        {
          m_pendingGroups.push_back(Size3(i, j, k));
        }
      }
    }
  }

  m_currentWorkGroup = NULL;
  m_currentWorkItem = NULL;
}

KernelInvocation::~KernelInvocation()
{
  // Destroy any remaining work-groups
  while (!m_runningGroups.empty())
  {
    delete m_runningGroups.front();
    m_runningGroups.pop_front();
  }
  if (m_currentWorkGroup)
  {
    delete m_currentWorkGroup;
    m_currentWorkGroup = NULL;
  }
}

const Context* KernelInvocation::getContext() const
{
  return m_context;
}

const WorkGroup* KernelInvocation::getCurrentWorkGroup() const
{
  return m_currentWorkGroup;
}

const WorkItem* KernelInvocation::getCurrentWorkItem() const
{
  return m_currentWorkItem;
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

bool KernelInvocation::nextWorkItem()
{
  m_currentWorkItem = NULL;
  if (m_currentWorkGroup)
  {
    // Switch to next ready work-item
    m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
    if (m_currentWorkItem)
    {
      return true;
    }

    // No work-items in READY state
    // Check if there are work-items at a barrier
    if (m_currentWorkGroup->hasBarrier())
    {
      // Resume execution
      m_currentWorkGroup->clearBarrier();
      m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
      return true;
    }

    // All work-items must have finished, destroy work-group
    m_context->notifyWorkGroupComplete(m_currentWorkGroup);
    delete m_currentWorkGroup;
    m_currentWorkGroup = NULL;
  }

  // Switch to next work-group
  if (!m_runningGroups.empty())
  {
    // Take work-group from running pool
    m_currentWorkGroup = m_runningGroups.front();
    m_runningGroups.pop_front();
  }
  else if (!m_pendingGroups.empty())
  {
    // Take work-group from pending pool
    Size3 group = m_pendingGroups.front();
    m_pendingGroups.pop_front();
    m_currentWorkGroup = new WorkGroup(this, group);
  }
  else
  {
    return false;
  }

  m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();

  // Check if this work-group has already finished
  if (!m_currentWorkItem)
  {
    return nextWorkItem();
  }

  return true;
}

void KernelInvocation::run(const Context *context, Kernel *kernel,
                           unsigned int workDim,
                           Size3 globalOffset,
                           Size3 globalSize,
                           Size3 localSize)
{
  try
  {
    // Allocate and initialise constant memory
    kernel->allocateConstants(context->getGlobalMemory());
  }
  catch (FatalError& err)
  {
    ostringstream info;
    info << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what()
         << endl << "When allocating kernel constants for '"
         << kernel->getName() << "'";
    context->logError(info.str().c_str());
    return;
  }

  // Create kernel invocation
  KernelInvocation *ki = new KernelInvocation(context, kernel, workDim,
                                              globalOffset,
                                              globalSize,
                                              localSize);

  context->notifyKernelBegin(ki);

  // Run kernel
  try
  {
    ki->run();
  }
  catch (FatalError& err)
  {
    ostringstream info;
    info << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what() << endl;
    context->logError(info.str().c_str());
  }

  context->notifyKernelEnd(ki);

  delete ki;

  // Deallocate constant memory
  kernel->deallocateConstants(context->getGlobalMemory());
}

void KernelInvocation::run()
{
  // Run until there are no more work-items
  while (nextWorkItem())
  {
    // Run current work-item as far as possible
    while (m_currentWorkItem->getState() == WorkItem::READY)
    {
      m_currentWorkItem->step();
    }
  }
}

bool KernelInvocation::switchWorkItem(const Size3 gid)
{
  // Compute work-group ID
  Size3 group(gid.x/m_localSize.x, gid.y/m_localSize.y, gid.z/m_localSize.z);

  bool found = false;
  WorkGroup *previousWorkGroup = m_currentWorkGroup;

  // Check if we're already running the work-group
  if (group == m_currentWorkGroup->getGroupID())
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
        m_currentWorkGroup = *rItr;
        m_runningGroups.erase(rItr);
        found = true;
        break;
      }
    }
  }

  // Check if work-group is in pending pool
  if (!found)
  {
    std::list<Size3>::iterator pItr;
    for (pItr = m_pendingGroups.begin(); pItr != m_pendingGroups.end(); pItr++)
    {
      if (group == *pItr)
      {
        m_currentWorkGroup = new WorkGroup(this, group);
        m_pendingGroups.erase(pItr);
        found = true;
        break;
      }
    }
  }

  if (!found)
  {
    return false;
  }

  if (previousWorkGroup != m_currentWorkGroup)
  {
    m_runningGroups.push_back(previousWorkGroup);
  }

  // Get work-item
  Size3 lid(gid.x%m_localSize.x, gid.y%m_localSize.y, gid.z%m_localSize.z);
  m_currentWorkItem = m_currentWorkGroup->getWorkItem(lid);

  return true;
}
