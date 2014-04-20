// Simulation.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include <cassert>
#include <fstream>
#include <iostream>

#include "oclgrind-kernel/Simulation.h"
#include "spirsim/Device.h"
#include "spirsim/Kernel.h"
#include "spirsim/Memory.h"
#include "spirsim/Program.h"

using namespace oclgrind;
using namespace std;

Simulation::Simulation()
{
  m_device = new Device();
  m_kernel = NULL;
  m_program = NULL;
}

Simulation::~Simulation()
{
  delete m_device;
  delete m_kernel;
  delete m_program;
}

bool Simulation::load(string filename)
{
  // Open simulator file
  ifstream input;
  input.open(filename);
  if (input.fail())
  {
    cerr << "Unable to open simulator file." << endl;
    return false;
  }

  // Read simulation parameters
  string progFileName;
  string kernelName;
  input >> progFileName
        >> kernelName
        >> m_ndrange[0] >> m_ndrange[1] >> m_ndrange[2]
        >> m_wgsize[0] >> m_wgsize[1] >> m_wgsize[2];

  // Ensure work-group size exactly divides NDRange
  if (m_ndrange[0] % m_wgsize[0] ||
      m_ndrange[1] % m_wgsize[1] ||
      m_ndrange[2] % m_wgsize[2])
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
    m_program = Program::createFromBitcodeFile(progFileName);
    if (!m_program)
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
    m_program = new Program(data);
    delete[] data;

    // Build program
    if (!m_program->build(""))
    {
      cerr << "Build failure:" << endl << m_program->getBuildLog() << endl;
      return false;
    }
  }

  // Get kernel
  m_kernel = m_program->createKernel(kernelName);
  if (!m_kernel)
  {
    cerr << "Failed to create kernel " << kernelName << endl;
    return false;
  }

  // Clear global memory
  Memory *globalMemory = m_device->getGlobalMemory();
  globalMemory->clear();

  // Set kernel arguments
  for (int idx = 0; idx < m_kernel->getNumArguments(); idx++)
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

    m_kernel->setArgument(idx, value);
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

void Simulation::run(bool dumpGlobalMemory)
{
  assert(m_kernel && m_program);
  assert(m_kernel->allArgumentsSet());

  size_t offset[] = {0, 0, 0};
  m_device->run(*m_kernel, 3, offset, m_ndrange, m_wgsize);

  if (dumpGlobalMemory)
  {
    cout << "Global Memory:" << endl;
    m_device->getGlobalMemory()->dump();
  }
}
