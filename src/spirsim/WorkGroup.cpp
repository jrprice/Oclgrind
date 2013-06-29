#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Constants.h"
#include "llvm/DebugInfo.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Type.h"

#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

WorkGroup::WorkGroup(const Kernel& kernel, Memory& globalMem,
                     size_t wgid_x, size_t wgid_y, size_t wgid_z,
                     const size_t globalSize[3],
                     const size_t groupSize[3])
  : m_globalMemory(globalMem)
{
  m_groupID[0] = wgid_x;
  m_groupID[1] = wgid_y;
  m_groupID[2] = wgid_z;
  m_globalSize[0] = globalSize[0];
  m_globalSize[1] = globalSize[1];
  m_globalSize[2] = globalSize[2];
  m_groupSize[0] = groupSize[0];
  m_groupSize[1] = groupSize[1];
  m_groupSize[2] = groupSize[2];

  // Allocate local memory
  m_localMemory = new Memory();
  m_localMemory->allocateBuffer(kernel.getLocalMemorySize());

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

void WorkGroup::dumpLocalMemory() const
{
  if (m_localMemory->getSize() > 0)
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

void WorkGroup::run(const Kernel& kernel, bool outputInstructions)
{
  const llvm::Function *function = kernel.getFunction();

  // Run until all work-items have finished
  int numFinished = 0;
  while (numFinished < m_totalWorkItems)
  {
    // Run work-items in order
    int numBarriers = 0;
    for (int i = 0; i < m_totalWorkItems; i++)
    {
      // Check if work-item is ready to execute
      WorkItem *workItem = m_workItems[i];
      if (workItem->getState() != WorkItem::READY)
      {
        continue;
      }

      // Debug output
      if (outputInstructions)
      {
        cout << SMALL_SEPARATOR << endl;
        const size_t *gid = m_workItems[i]->getGlobalID();
        cout << "Work-item ("
             << gid[0] << "," << gid[1] << "," << gid[2]
             << "):" << endl;
      }

      // Run work-item until barrier or complete
      WorkItem::State state = workItem->getState();
      while (state == WorkItem::READY)
      {
        state = workItem->step(outputInstructions);
      }

      // Update counters
      if (state == WorkItem::BARRIER)
      {
        numBarriers++;
        if (outputInstructions)
        {
          cout << SMALL_SEPARATOR << endl;
          cout << "Barrier reached." << endl;
        }
      }
      else if (state == WorkItem::FINISHED)
      {
        numFinished++;
        if (outputInstructions)
        {
          cout << SMALL_SEPARATOR << endl;
          cout << "Kernel completed." << endl;
        }
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
      if (outputInstructions)
      {
        cout << "All work-items reached barrier." << endl;
      }
    }
    else if (numBarriers > 0)
    {
      cout << "Barrier divergence detected." << endl;
      return;
    }
  }

  if (outputInstructions)
  {
    cout << "All work-items completed kernel." << endl;
  }
}
