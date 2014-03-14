// main.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include <fstream>
#include <string>

#include "spirsim/Device.h"
#include "spirsim/Kernel.h"
#include "spirsim/Memory.h"
#include "spirsim/Program.h"

using namespace spirsim;
using namespace std;

static bool outputGlobalMemory = false;
static const char *simfile = NULL;

static size_t ndrange[3];
static size_t wgsize[3];
static Device *device = NULL;
static Program *program = NULL;
static Kernel *kernel = NULL;

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
    cerr << "Unable to open simulator file." << endl;
    return 1;
  }

  // Initialise simulator
  device = new Device();
  bool ret = init(input);
  input.close();
  if (!ret)
  {
    return 1;
  }

  // Run kernel
  size_t offset[3] = {0,0,0};
  device->run(*kernel, 3, offset, ndrange, wgsize);
  if (outputGlobalMemory)
  {
    cout << "Global Memory:" << endl;
    device->getGlobalMemory()->dump();
  }

  delete device;
  delete kernel;
  delete program;
}

bool init(istream& input)
{
  string progFileName;
  string kernelName;

  // Read simulation parameters
  input >> progFileName
        >> kernelName
        >> ndrange[0] >> ndrange[1] >> ndrange[2]
        >> wgsize[0] >> wgsize[1] >> wgsize[2];

  // Ensure work-group size exactly divides NDRange
  if (ndrange[0] % wgsize[0] ||
      ndrange[1] % wgsize[1] ||
      ndrange[2] % wgsize[2])
  {
    cerr << "Work group size must divide NDRange exactly." << endl;
    return false;
  }

  // Open program file
  ifstream progFile;
  progFile.open(progFileName.c_str(), ios_base::in | ios_base::binary);
  if (!progFile.good())
  {
    cerr << "Unable to open " << progFileName << endl;
    return false;
  }

  // Check for LLVM bitcode magic numbers
  char magic[2] = {0,0};
  progFile.read(magic, 2);
  if (magic[0] == 0x42 && magic[1] == 0x43)
  {
    // Load bitcode
    progFile.close();
    program = Program::createFromBitcodeFile(progFileName);
    if (!program)
    {
      cerr << "Failed to load bitcode from " << progFileName << endl;
      return false;
    }
  }
  else
  {
    // Get size of file
    progFile.seekg(0, ios_base::end);
    size_t sz = progFile.tellg();
    progFile.seekg(0, ios_base::beg);

    // Load source
    char *data = new char[sz + 1];
    progFile.read(data, sz+1);
    progFile.close();
    data[sz] = '\0';
    program = new Program(data);
    delete[] data;

    // Build program
    if (!program->build(""))
    {
      cerr << "Build failure:" << endl << program->getBuildLog() << endl;
      return false;
    }
  }

  // Get kernel
  kernel = program->createKernel(kernelName);
  if (!kernel)
  {
    cerr << "Failed to create kernel " << kernelName << endl;
    return false;
  }

  // Clear global memory
  Memory *globalMemory = device->getGlobalMemory();
  globalMemory->clear();

  // Set kernel arguments
  for (int idx = 0; idx < kernel->getNumArguments(); idx++)
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
      cerr << "Error reading kernel arguments." << endl;
      return false;
    }

    switch (type)
    {
    case 'b':
      // Allocate buffer
      address = globalMemory->allocateBuffer(size);
      if (!address)
      {
        cerr << "Failed to allocate buffer" << endl;
        return false;
      }

      // Initialise buffer
      for (i = 0; i < size; i++)
      {
        input >> hex >> byte;
        globalMemory->store((unsigned char*)&byte, address + i);
      }

      // Set argument value
      value.size = sizeof(size_t);
      value.num = 1;
      value.data = new unsigned char[value.size];
      *((size_t*)value.data) = address;

      break;
    case 'l':
      // Allocate local memory argument
      value.size = size;
      value.num = 1;
      value.data = NULL;

      break;
    case 's':
      // Create scalar argument
      value.size = size;
      value.num = 1;
      value.data = new unsigned char[value.size];
      for (i = 0; i < size; i++)
      {
        input >> hex >> byte;
        value.data[i] = (unsigned char)byte;
      }

      break;
    default:
      cerr << "Unrecognised argument type '" << type << "'" << endl;
      return false;
    }

    kernel->setArgument(idx, value);
    if (value.data)
    {
      delete[] value.data;
    }
  }

  // Make sure there is no more input
  std::string next;
  input >> next;
  if (input.good() || !input.eof())
  {
    cerr << "Unexpected token '" << next << "' (expected EOF)" << endl;
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
          outputGlobalMemory = true;
          break;
        default:
          cerr << "Unrecognised option '" << argv[i] << "'" << endl;
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
        cerr << "Unexpected positional argument '" << argv[i] << "'" << endl;
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
  cout << "Usage: oclgrind-kernel [-g] simfile" << endl;
}
