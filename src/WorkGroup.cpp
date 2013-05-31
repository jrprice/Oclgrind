#include "common.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/DebugInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

#include "Kernel.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace std;

WorkGroup::WorkGroup(const Kernel& kernel, Memory& globalMem,
                     size_t wgid_x, size_t wgid_y, size_t wgid_z,
                     size_t groupSize[3])
  : m_globalMemory(globalMem)
{
  m_groupID[0] = wgid_x;
  m_groupID[1] = wgid_y;
  m_groupID[2] = wgid_z;
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
        WorkItem *workItem = new WorkItem(*this, kernel, globalMem,
                                          wgid_x*groupSize[0] + i,
                                          wgid_y*groupSize[1] + j,
                                          wgid_z*groupSize[2] + k);
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

const size_t* WorkGroup::getGroupID() const
{
  return m_groupID;
}

Memory* WorkGroup::getLocalMemory() const
{
  return m_localMemory;
}

void WorkGroup::run(const llvm::Function *function, bool outputInstructions)
{
  // Iterate over work-items
  // TODO: Non-sequential work-item execution
  for (int i = 0; i < m_totalWorkItems; i++)
  {
    // Debug output
    if (outputInstructions)
    {
      cout << SMALL_SEPARATOR << endl;
      const size_t *gid = m_workItems[i]->getGlobalID();
      cout << "Work-item ("
           << gid[0] << ","
           << gid[1] << ","
           << gid[2]
           << ") Instructions:" << endl;
    }

    // Iterate over basic blocks in function
    llvm::Function::const_iterator blockItr;
    for (blockItr = function->begin(); blockItr != function->end();)
    {
      m_workItems[i]->setCurrentBlock(blockItr);

      // Iterate over instructions in block
      llvm::BasicBlock::const_iterator instItr;
      for (instItr = blockItr->begin(); instItr != blockItr->end(); instItr++)
      {
        if (outputInstructions)
        {
          dumpInstruction(*instItr, true);
        }
        m_workItems[i]->execute(*instItr);
      }

      // Get next block
      if (m_workItems[i]->getNextBlock() == NULL)
      {
        // TODO: Cleaner way of handling ret terminator
        break;
      }
      else
      {
        blockItr = (const llvm::BasicBlock*)(m_workItems[i]->getNextBlock());
      }
    }
  }
}
