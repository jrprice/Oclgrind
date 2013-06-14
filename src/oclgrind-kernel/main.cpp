#include <fstream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/Scalar.h"

#include "spirsim/Kernel.h"
#include "spirsim/Memory.h"
#include "spirsim/Simulator.h"

using namespace std;

static unsigned char outputMask = 0;
static const char *simfile = NULL;

static llvm::LLVMContext context;
static size_t ndrange[3];
static size_t wgsize[3];
static Kernel *kernel = NULL;
static Simulator *simulator = NULL;

static bool init(istream& input);
static bool parseArguments(int argc, char *argv[]);
static void printUsage();

int main(int argc, char *argv[])
{
  // Parse arguments
  if (!parseArguments(argc, argv))
  {
    return 1;
  }

  // Attempt to open simulator file
  ifstream input;
  input.open(simfile);
  if (input.fail())
  {
    cout << "Unable to open simulator file." << endl;
    return 1;
  }

  // Initialise simulator
  simulator = new Simulator();
  bool ret = init(input);
  input.close();
  if (!ret)
  {
    return 1;
  }

  // Run simulator
  simulator->setOutputMask(outputMask);
  simulator->run(*kernel, ndrange, wgsize);
  delete simulator;
}

bool init(istream& input)
{
  string spir;
  string kernelName;

  // Read simulation parameters
  input >> spir
        >> kernelName
        >> ndrange[0] >> ndrange[1] >> ndrange[2]
        >> wgsize[0] >> wgsize[1] >> wgsize[2];

  // Ensure work-group size exactly divides NDRange
  if (ndrange[0] % wgsize[0] ||
      ndrange[1] % wgsize[1] ||
      ndrange[2] % wgsize[2])
  {
    cout << "Work group size must divide NDRange exactly." << endl;
    return false;
  }

  // Load IR module from file
  llvm::SMDiagnostic err;
  llvm::Module* module = ParseIRFile(spir, err, context);
  if (!module)
  {
    cout << "Failed to load SPIR." << endl;
    return false;
  }

  // Iterate over functions in module to find kernel
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

  kernel = new Kernel(function);
  kernel->setGlobalSize(ndrange);

  // Clear global memory
  Memory *globalMemory = simulator->getGlobalMemory();
  globalMemory->clear();

  // Set kernel arguments
  const llvm::Function::ArgumentListType& args = function->getArgumentList();
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
      address = globalMemory->allocateBuffer(size);

      // Initialise buffer
      for (i = 0; i < size; i++)
      {
        input >> hex >> byte;
        globalMemory->store(address + i, (unsigned char)byte);
      }

      // TODO: Address assignment
      value.size = sizeof(size_t);
      value.data = new unsigned char[value.size];
      *((size_t*)value.data) = address;

      break;
    case 'l':
      // Allocate local memory argument
      address = kernel->allocateLocalMemory(size);
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

    kernel->setArgument(argItr, value);
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

static bool parseArguments(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
    {
      char *opt = argv[i] + 1;
      while (*opt != '\0')
      {
        switch (*opt)
        {
        case 'g':
          outputMask |= Simulator::OUTPUT_GLOBAL_MEM;
          break;
        case 'l':
          outputMask |= Simulator::OUTPUT_LOCAL_MEM;
          break;
        case 'p':
          outputMask |= Simulator::OUTPUT_PRIVATE_MEM;
          break;
        case 'i':
          outputMask |= Simulator::OUTPUT_INSTRUCTIONS;
          break;
        default:
          cout << "Unrecognised option '" << argv[i] << "'" << endl;
          return false;
        }
        opt++;
      }
    }
    else
    {
      if (simfile == NULL)
      {
        simfile = argv[i];
      }
      else
      {
        cout << "Unexpected positional argument '" << argv[i] << "'" << endl;
        return false;
      }
    }
  }

  if (simfile == NULL)
  {
    printUsage();
    return false;
  }

  return true;
}

static void printUsage()
{
  cout << "Usage: oclgrind [-g] [-p] [-i] simfile" << endl;
}
