// main.cpp (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#include <fstream>

#include "spirsim/Device.h"
#include "spirsim/Kernel.h"
#include "spirsim/Memory.h"
#include "spirsim/Program.h"

using namespace spirsim;
using namespace std;

static unsigned char outputMask = 0;
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
    cout << "Unable to open simulator file." << endl;
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

  // Run device
  device->setOutputMask(outputMask);
  device->run(*kernel, 3, NULL, ndrange, wgsize);
  delete device;
  delete kernel;
  delete program;
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

  program = Program::createFromBitcodeFile(spir);
  if (!program)
  {
    return false;
  }

  kernel = program->createKernel(kernelName);

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
      cout << "Error reading kernel arguments." << endl;
      return false;
    }

    switch (type)
    {
    case 'b':
      // Allocate buffer
      address = globalMemory->allocateBuffer(size);
      if (!address)
      {
        cout << "Failed to allocate buffer" << endl;
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
      cout << "Unrecognised argument type '" << type << "'" << endl;
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
          outputMask |= Device::OUTPUT_GLOBAL_MEM;
          break;
        case 'l':
          outputMask |= Device::OUTPUT_LOCAL_MEM;
          break;
        case 'p':
          outputMask |= Device::OUTPUT_PRIVATE_MEM;
          break;
        case 'i':
          outputMask |= Device::OUTPUT_INSTRUCTIONS;
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
