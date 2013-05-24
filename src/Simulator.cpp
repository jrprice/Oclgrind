#include "common.h"
#include <istream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/SourceMgr.h"

#include "GlobalMemory.h"
#include "Kernel.h"
#include "Simulator.h"
#include "WorkItem.h"

using namespace std;

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

  input >> spir
        >> kernelName
        >> m_ndrange[0] >> m_ndrange[1] >> m_ndrange[2]
        >> m_wgsize[0] >> m_wgsize[1] >> m_wgsize[2];

  // Load IR module from file
  llvm::SMDiagnostic err;
  m_module = ParseIRFile(spir, err, *m_context);
  if (!m_module)
  {
    cout << "Failed to load SPIR." << endl;
    return false;
  }

  // Iterate over functions in module to find kernel
  m_function = NULL;
  llvm::Module::const_iterator fitr;
  for(fitr = m_module->begin(); fitr != m_module->end(); fitr++)
  {
    // Check kernel name
    if (fitr->getName().str() != kernelName)
      continue;

    m_function = fitr;
    break;
  }
  if (m_function == NULL)
  {
    cout << "Failed to locate kernel." << endl;
    return false;
  }

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

    input >> type >> size;

    switch (type)
    {
    case 'b':
      // Allocate buffer
      address = m_globalMemory->allocateBuffer(size);

      // Initialise buffer
      for (i = 0; i < size; i++)
      {
        input >> byte;
        m_globalMemory->store(address + i, (unsigned char)byte);
      }

      // TODO: Pointer size
      // TODO: Address assignment
      value.size = 4;
      value.data = new unsigned char[value.size];
      *value.data = address;

      break;
    case 's':
      // Create scalar argument
      value.size = size;
      value.data = new unsigned char[value.size];
      for (i = 0; i < size; i++)
      {
        input >> byte;
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
  WorkItem *workItems[totalWorkItems];
  for (int k = 0; k < m_ndrange[2]; k++)
  {
    for (int j = 0; j < m_ndrange[1]; j++)
    {
      for (int i = 0; i < m_ndrange[0]; i++)
      {
        workItems[i + (j + k*m_ndrange[1])*m_ndrange[0]] =
          new WorkItem(*m_kernel, *m_globalMemory, i, j, k);
      }
    }
  }
  workItems[0]->enableDebugOutput(true);

  // Iterate over instructions in function
  // TODO: Implement non-linear control flow
  llvm::const_inst_iterator iitr;
  for (iitr = inst_begin(m_function); iitr != inst_end(m_function); iitr++)
  {
    for (int i = 0; i < totalWorkItems; i++)
    {
      workItems[i]->execute(*iitr);
    }
  }

  // Temporarily dump memories (TODO: Remove)
  workItems[0]->dumpPrivateMemory();
  m_globalMemory->dump();
  for (int i = 0; i < totalWorkItems; i++)
  {
    delete workItems[i];
  }
}
