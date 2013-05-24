#include "config.h"
#include <iostream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Module.h"
#include "llvm/Support/InstIterator.h"

#include "Simulator.h"
#include "WorkItem.h"

using namespace std;

Simulator::Simulator(const llvm::Module *module)
{
  m_module = module;
}

void Simulator::run(string kernel)
{
  llvm::Module::const_iterator fitr;
  llvm::const_inst_iterator iitr;

  // TODO: Multiple work-items
  WorkItem workItem(0);

  // Iterate over functions in module
  for(fitr = m_module->begin(); fitr != m_module->end(); fitr++)
  {
    // Check kernel name
    if (fitr->getName().str() != kernel)
      continue;

    // Iterate over instructions in function
    // TODO: Implement non-linear control flow
    for (iitr = inst_begin(fitr); iitr != inst_end(fitr); iitr++)
    {
      workItem.execute(*iitr);
    }
  }

  // Temporarily dump private memory (TODO: Remove)
  workItem.dumpPrivateMemory();
}
