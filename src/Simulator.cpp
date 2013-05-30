#include "common.h"
#include <istream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/Scalar.h"

#include "GlobalMemory.h"
#include "Kernel.h"
#include "Simulator.h"
#include "WorkItem.h"

using namespace std;

#define SEPARATOR "--------------------------------"

Simulator::Simulator()
{
  m_context = new llvm::LLVMContext;

  m_globalMemory = new GlobalMemory();
  m_kernel = new Kernel();
}

Simulator::~Simulator()
{
  delete m_globalMemory;
  delete m_kernel;
}

bool Simulator::init(istream& input)
{
  string spir;
  string kernelName;

  // Read simulation parameters
  input >> spir
        >> kernelName
        >> m_ndrange[0] >> m_ndrange[1] >> m_ndrange[2]
        >> m_wgsize[0] >> m_wgsize[1] >> m_wgsize[2];

  // Load IR module from file
  llvm::SMDiagnostic err;
  llvm::Module* module = ParseIRFile(spir, err, *m_context);
  if (!module)
  {
    cout << "Failed to load SPIR." << endl;
    return false;
  }

  // Iterate over functions in module to find kernel
  m_function = NULL;
  llvm::Function *function = NULL;
  llvm::Module::iterator fitr;
  for(fitr = module->begin(); fitr != module->end(); fitr++)
  {
    // Check kernel name
    if (fitr->getName().str() != kernelName)
      continue;

    function = fitr;

    break;
  }
  if (function == NULL)
  {
    cout << "Failed to locate kernel." << endl;
    return false;
  }

  // Assign identifiers to unnamed temporaries
  llvm::FunctionPass *instNamer = llvm::createInstructionNamerPass();
  instNamer->runOnFunction(*((llvm::Function*)function));
  delete instNamer;

  m_module = module;
  m_function = function;

  // Clear global memory
  m_globalMemory->clear();

  // Set kernel arguments
  const llvm::Function::ArgumentListType& args = m_function->getArgumentList();
  llvm::Function::const_arg_iterator aitr;
  for (aitr = args.begin(); aitr != args.end(); aitr++)
  {
    char type;
    size_t size;
    size_t address;
    int i;
    int byte;
    TypedValue value;

    input >> type >> dec >> size;

    switch (type)
    {
    case 'b':
      // Allocate buffer
      address = m_globalMemory->allocateBuffer(size);

      // Initialise buffer
      for (i = 0; i < size; i++)
      {
        input >> hex >> byte;
        m_globalMemory->store(address + i, (unsigned char)byte);
      }

      // TODO: Address assignment
      value.size = sizeof(size_t);
      value.data = new unsigned char[value.size];
      *((size_t*)value.data) = address;

      break;
    case 's':
      // Create scalar argument
      value.size = size;
      value.data = new unsigned char[value.size];
      for (i = 0; i < size; i++)
      {
        input >> hex >> byte;
        value.data[i] = (unsigned char)byte;
      }

      break;
    default:
      cout << "Unrecognised argument type '" << type << "'" << endl;
      return false;
    }

    m_kernel->setArgument(aitr, value);
  }

  // Make sure there is no more input
  std::string next;
  input >> next;
  if (input.good() || !input.eof())
  {
    cout << "Unexpected token '" << next << "' (expected EOF)" << endl;
    return false;
  }

  return true;
}

void Simulator::run()
{
  // TODO: Work-groups

  // Initialise work-items
  size_t totalWorkItems = m_ndrange[0] * m_ndrange[1] * m_ndrange[2];
  WorkItem **workItems = new WorkItem*[totalWorkItems];
  for (int k = 0; k < m_ndrange[2]; k++)
  {
    for (int j = 0; j < m_ndrange[1]; j++)
    {
      for (int i = 0; i < m_ndrange[0]; i++)
      {
        WorkItem *workItem = new WorkItem(*m_kernel, *m_globalMemory, i, j, k);
        workItem->enableDebugOutput(m_outputMask & OUTPUT_INSTRUCTIONS);
        workItems[i + (j + k*m_ndrange[1])*m_ndrange[0]] = workItem;
      }
    }
  }

  // Iterate over work-items
  // TODO: Non-sequential work-item execution
  for (int i = 0; i < totalWorkItems; i++)
  {
    if (m_outputMask & (OUTPUT_PRIVATE_MEM | OUTPUT_INSTRUCTIONS))
    {
      cout << SEPARATOR << endl;
    }
    if (m_outputMask & OUTPUT_INSTRUCTIONS)
    {
      const size_t *gid = workItems[i]->getGlobalID();
      cout << "Work-item ("
           << gid[0] << ","
           << gid[1] << ","
           << gid[2]
           << ") Instructions:" << endl;
    }

    // Iterate over basic blocks in function
    llvm::Function::const_iterator bitr;
    for (bitr = m_function->begin(); bitr != m_function->end();)
    {
      workItems[i]->setCurrentBlock(bitr);

      // Iterate over instructions in block
      llvm::BasicBlock::const_iterator iitr;
      for (iitr = bitr->begin(); iitr != bitr->end(); iitr++)
      {
        workItems[i]->execute(*iitr);
      }

      // Get next block
      if (workItems[i]->getNextBlock() == NULL)
      {
        // TODO: Cleaner way of handling ret terminator
        break;
      }
      else
      {
        bitr = (const llvm::BasicBlock*)(workItems[i]->getNextBlock());
      }
    }

    if (m_outputMask & OUTPUT_PRIVATE_MEM)
    {
      workItems[i]->dumpPrivateMemory();
    }

    if (m_outputMask & (OUTPUT_PRIVATE_MEM | OUTPUT_INSTRUCTIONS))
    {
      cout << SEPARATOR << endl;
    }
  }

  // Delete work-items and output private memory dump if required
  for (int i = 0; i < totalWorkItems; i++)
  {
    delete workItems[i];
  }
  delete[] workItems;

  // Output global memory dump if required
  if (m_outputMask & OUTPUT_GLOBAL_MEM)
  {
    m_globalMemory->dump();
  }
}

void Simulator::setOutputMask(unsigned char mask)
{
  m_outputMask = mask;
}
