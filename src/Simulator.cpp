#include "config.h"
#include <iostream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/Module.h"
#include "llvm/Support/InstIterator.h"

#include "GlobalMemory.h"
#include "Kernel.h"
#include "Simulator.h"
#include "WorkItem.h"

using namespace std;

Simulator::Simulator(const llvm::Module *module)
{
  m_module = module;
}

void Simulator::run(string kernelName)
{
  llvm::Module::const_iterator fitr;
  llvm::const_inst_iterator iitr;

  // TODO: Dynamically set
  int globalSize = 4;

  // Create global memory
  // TODO: Allocate/initialise dynamically
  GlobalMemory globalMemory;
  size_t a = globalMemory.allocateBuffer(globalSize*sizeof(float));
  size_t b = globalMemory.allocateBuffer(globalSize*sizeof(float));
  size_t c = globalMemory.allocateBuffer(globalSize*sizeof(float));
  for (int i = 0; i < globalSize; i++)
  {
    globalMemory.store(a+i*4, i);
    globalMemory.store(b+i*4, i);
  }

  Kernel kernel;

  WorkItem *workItems[globalSize];

  // Iterate over functions in module
  for(fitr = m_module->begin(); fitr != m_module->end(); fitr++)
  {
    // Check kernel name
    if (fitr->getName().str() != kernelName)
      continue;

    // Set kernel arguments
    // TODO: Set these dynamically
    const llvm::Function::ArgumentListType& args = fitr->getArgumentList();
    llvm::Function::const_arg_iterator aitr = args.begin();
    kernel.setArgument(aitr++, a);
    kernel.setArgument(aitr++, b);
    kernel.setArgument(aitr++, c);

    // Initialise work-items
    for (int i = 0; i < globalSize; i++)
    {
      workItems[i] = new WorkItem(kernel, globalMemory, i);
    }
    workItems[0]->enableDebugOutput(true);

    // Iterate over instructions in function
    // TODO: Implement non-linear control flow
    for (iitr = inst_begin(fitr); iitr != inst_end(fitr); iitr++)
    {
      for (int i = 0; i < globalSize; i++)
      {
        workItems[i]->execute(*iitr);
      }
    }
  }

  // Temporarily dump memories (TODO: Remove)
  workItems[0]->dumpPrivateMemory();
  globalMemory.dump();
  for (int i = 0; i < globalSize; i++)
  {
    delete workItems[i];
  }
}
