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

#include "Kernel.h"
#include "Memory.h"
#include "Simulator.h"
#include "WorkGroup.h"

using namespace std;

Simulator::Simulator()
{
  m_context = new llvm::LLVMContext;

  m_globalMemory = new Memory();
  m_kernel = NULL;
}

Simulator::~Simulator()
{
  delete m_globalMemory;
  if (m_kernel)
  {
    delete m_kernel;
  }
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

  // Ensure work-group size exactly divides NDRange
  if (m_ndrange[0] % m_wgsize[0] ||
      m_ndrange[1] % m_wgsize[1] ||
      m_ndrange[2] % m_wgsize[2])
  {
    cout << "Work group size must divide NDRange exactly." << endl;
    return false;
  }

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
  llvm::Module::iterator funcItr;
  for(funcItr = module->begin(); funcItr != module->end(); funcItr++)
  {
    // Check kernel name
    if (funcItr->getName().str() != kernelName)
      continue;

    function = funcItr;

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
  m_kernel = new Kernel(function);
  m_kernel->setGlobalSize(m_ndrange);

  // Clear global memory
  m_globalMemory->clear();

  // Set kernel arguments
  const llvm::Function::ArgumentListType& args = m_function->getArgumentList();
  llvm::Function::const_arg_iterator argItr;
  for (argItr = args.begin(); argItr != args.end(); argItr++)
  {
    char type;
    size_t size;
    size_t address;
    int i;
    int byte;
    TypedValue value;

    input >> type >> dec >> size;
    if (input.fail())
    {
      cout << "Error reading kernel arguments." << endl;
      return false;
    }

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
    case 'l':
      // Allocate local memory argument
      address = m_kernel->allocateLocalMemory(size);
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

    m_kernel->setArgument(argItr, value);
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
  // Create work-groups
  size_t numGroups[3] = {m_ndrange[0]/m_wgsize[0],
                         m_ndrange[1]/m_wgsize[1],
                         m_ndrange[2]/m_wgsize[2]};
  size_t totalNumGroups = numGroups[0]*numGroups[1]*numGroups[2];
  for (int k = 0; k < numGroups[2]; k++)
  {
    for (int j = 0; j < numGroups[1]; j++)
    {
      for (int i = 0; i < numGroups[0]; i++)
      {
        if (m_outputMask &
            (OUTPUT_INSTRUCTIONS | OUTPUT_PRIVATE_MEM | OUTPUT_LOCAL_MEM))
        {
          cout << endl << BIG_SEPARATOR << endl;
          cout << "Work-group ("
               << i << ","
               << j << ","
               << k
               << ")" << endl;
          cout << BIG_SEPARATOR << endl;
        }

        WorkGroup *workGroup = new WorkGroup(*m_kernel, *m_globalMemory,
                                             i, j, k, m_wgsize);

        workGroup->run(m_function, m_outputMask & OUTPUT_INSTRUCTIONS);

        // Dump contents of memories
        if (m_outputMask & OUTPUT_PRIVATE_MEM)
        {
          workGroup->dumpPrivateMemory();
        }
        if (m_outputMask & OUTPUT_LOCAL_MEM)
        {
          workGroup->dumpLocalMemory();
        }

        delete workGroup;
      }
    }
  }

  // Output global memory dump if required
  if (m_outputMask & OUTPUT_GLOBAL_MEM)
  {
    cout << endl << BIG_SEPARATOR << endl << "Global Memory:";
    m_globalMemory->dump();
    cout << BIG_SEPARATOR << endl;
  }
}

void Simulator::setOutputMask(unsigned char mask)
{
  m_outputMask = mask;
}
