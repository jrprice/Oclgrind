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

  // Create global memory
  // TODO: Allocate dynamically
  GlobalMemory globalMemory;
  size_t a = globalMemory.allocateBuffer(4);
  size_t b = globalMemory.allocateBuffer(4);
  size_t c = globalMemory.allocateBuffer(4);
  globalMemory.store(a, 0x13);
  globalMemory.store(b, 0x2F);

  Kernel kernel;

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

    // TODO: Multiple work-items
    WorkItem workItem(kernel, globalMemory, 0);

    // Iterate over instructions in function
    // TODO: Implement non-linear control flow
    for (iitr = inst_begin(fitr); iitr != inst_end(fitr); iitr++)
    {
      workItem.execute(*iitr);
    }

    // Temporarily dump private memory (TODO: Remove)
    workItem.dumpPrivateMemory();
    globalMemory.dump();
  }
}
