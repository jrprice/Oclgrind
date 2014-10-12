// Device.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <istream>
#include <iterator>
#include <sstream>

#if HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "llvm/DebugInfo.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"

#include "Device.h"
#include "Context.h"
#include "Kernel.h"
#include "Memory.h"
#include "Plugin.h"
#include "Program.h"
#include "WorkGroup.h"
#include "WorkItem.h"

#include "plugins/InstructionCounter.h"
#include "plugins/RaceDetector.h"

using namespace oclgrind;
using namespace std;

#define LIST_LENGTH 10

Device::Device(const Context *context)
  : m_context(context)
{
  m_kernelInvocation = NULL;
  m_currentWorkGroup = NULL;
  m_currentWorkItem = NULL;

  // Check for quick-mode environment variable
  const char *quick = getenv("OCLGRIND_QUICK");
  m_quickMode = (quick && strcmp(quick, "1") == 0);
}

Device::~Device()
{
}

WorkGroup* Device::createWorkGroup(size_t x, size_t y, size_t z)
{
  return new WorkGroup(m_context, m_kernelInvocation, x, y, z);
}

const WorkGroup* Device::getCurrentWorkGroup() const
{
  return m_currentWorkGroup;
}

const WorkItem* Device::getCurrentWorkItem() const
{
  return m_currentWorkItem;
}

const KernelInvocation* Device::getCurrentKernelInvocation() const
{
  return m_kernelInvocation;
}

bool Device::nextWorkItem()
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
    PendingWorkGroup group = m_pendingGroups.front();
    m_pendingGroups.pop_front();
    m_currentWorkGroup = createWorkGroup(group[0], group[1], group[2]);
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

void Device::run(Kernel *kernel, unsigned int workDim,
                 const size_t *globalOffset,
                 const size_t *globalSize,
                 const size_t *localSize)
{
  assert(m_runningGroups.empty());

  KernelInvocation *ki = new KernelInvocation;

  // Set-up offsets and sizes
  ki->workDim         = workDim;
  ki->globalSize[0]   = ki->globalSize[1]   = ki->globalSize[2]   = 1;
  ki->globalOffset[0] = ki->globalOffset[1] = ki->globalOffset[2] = 0;
  ki->localSize[0]    = ki->localSize[1]    = ki->localSize[2]    = 1;
  for (int i = 0; i < workDim; i++)
  {
    ki->globalSize[i] = globalSize[i];
    if (globalOffset[i])
    {
      ki->globalOffset[i] = globalOffset[i];
    }
    if (localSize[i])
    {
      ki->localSize[i] = localSize[i];
    }
  }

  try
  {
    // Allocate and initialise constant memory
    kernel->allocateConstants(m_context->getGlobalMemory());
  }
  catch (FatalError& err)
  {
    ostringstream info;
    info << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what()
         << endl << "When allocating kernel constants for '"
         << kernel->getName() << "'";
    m_context->logError(info.str().c_str());
    return;
  }

  // Create pool of pending work-groups
  ki->numGroups[0] = ki->globalSize[0]/ki->localSize[0];
  ki->numGroups[1] = ki->globalSize[1]/ki->localSize[1];
  ki->numGroups[2] = ki->globalSize[2]/ki->localSize[2];
  if (m_quickMode)
  {
    // Only run first and last work-groups in quick-mode
    PendingWorkGroup firstGroup = {{0, 0, 0}};
    PendingWorkGroup lastGroup  =
    {{
      ki->numGroups[0]-1,
      ki->numGroups[1]-1,
      ki->numGroups[2]-1
    }};
    m_pendingGroups.push_back(firstGroup);
    m_pendingGroups.push_back(lastGroup);
  }
  else
  {
    for (size_t k = 0; k < ki->numGroups[2]; k++)
    {
      for (size_t j = 0; j < ki->numGroups[1]; j++)
      {
        for (size_t i = 0; i < ki->numGroups[0]; i++)
        {
          PendingWorkGroup workGroup = {{i, j, k}};
          m_pendingGroups.push_back(workGroup);
        }
      }
    }
  }

  // Prepare kernel invocation
  ki->kernel = kernel;
  m_kernelInvocation = ki;
  m_currentWorkGroup = NULL;
  m_currentWorkItem = NULL;
  nextWorkItem();

  m_context->notifyKernelBegin(ki);

  try
  {
    while (m_currentWorkItem)
    {
      // Run current work-item as far as possible
      while (m_currentWorkItem->getState() == WorkItem::READY)
      {
        m_currentWorkItem->step();
      }
      nextWorkItem();
    }
  }
  catch (FatalError& err)
  {
    ostringstream info;
    info << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what() << endl;
    m_context->logError(info.str().c_str());
  }

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

  // Deallocate constant memory
  kernel->deallocateConstants(m_context->getGlobalMemory());

  m_context->notifyKernelEnd(ki);

  delete m_kernelInvocation;
  m_kernelInvocation = NULL;
}

bool Device::switchWorkItem(const size_t gid[3])
{
  // Compute work-group ID
  size_t group[3] =
  {
    gid[0]/m_kernelInvocation->localSize[0],
    gid[1]/m_kernelInvocation->localSize[1],
    gid[2]/m_kernelInvocation->localSize[2]
  };

  bool found = false;
  WorkGroup *previousWorkGroup = m_currentWorkGroup;

  // Check if we're already running the work-group
  const size_t *_group = m_currentWorkGroup->getGroupID();
  if (group[0] == _group[0] &&
      group[1] == _group[1] &&
      group[2] == _group[2])
  {
    found = true;
  }

  // Check if work-group is in running pool
  if (!found)
  {
    std::list<WorkGroup*>::iterator rItr;
    for (rItr = m_runningGroups.begin(); rItr != m_runningGroups.end(); rItr++)
    {
      const size_t *_group = (*rItr)->getGroupID();
      if (group[0] == _group[0] &&
          group[1] == _group[1] &&
          group[2] == _group[2])
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
    std::list<PendingWorkGroup>::iterator pItr;
    for (pItr = m_pendingGroups.begin(); pItr != m_pendingGroups.end(); pItr++)
    {
      const size_t *_group = pItr->group;
      if (group[0] == _group[0] &&
          group[1] == _group[1] &&
          group[2] == _group[2])
      {
        m_currentWorkGroup = createWorkGroup(group[0], group[1], group[2]);
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
  size_t lid[3] =
  {
    gid[0]%m_kernelInvocation->localSize[0],
    gid[1]%m_kernelInvocation->localSize[1],
    gid[2]%m_kernelInvocation->localSize[2]
  };
  m_currentWorkItem = m_currentWorkGroup->getWorkItem(lid);

  return true;
}
