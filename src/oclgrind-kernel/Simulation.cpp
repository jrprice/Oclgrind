// Simulation.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include <cassert>
#include <iostream>

#include "oclgrind-kernel/Simulation.h"
#include "spirsim/Device.h"
#include "spirsim/Kernel.h"
#include "spirsim/Memory.h"
#include "spirsim/Program.h"

#define PARSING(parsing) m_parsing = parsing;
#define PARSING_ARG(arg)                 \
  char parsing[256];                     \
  sprintf(parsing, "argument %d", arg);  \
  PARSING(parsing);

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

template<typename T> void Simulation::get(T& result)
{
  while (m_simfile.good())
  {
    // Attempt to read value
    m_simfile >> result;
    if (m_simfile.good())
    {
      break;
    }

    // Skip comments
    m_simfile.clear();
    if (m_simfile.peek() == '#')
    {
      m_simfile.ignore(UINT32_MAX, '\n');
    }
    else
    {
      throw ifstream::failbit;
    }

    // Throw exception at end of file
    if (m_simfile.eof())
    {
      throw ifstream::eofbit;
    }
  }
}

void Simulation::get(string& result)
{
  do
  {
    get<string>(result);

    // Remove comment from string
    size_t comment = result.find_first_of('#');
    if (comment != string::npos)
    {
      result = result.substr(0, comment);
      m_simfile.ignore(UINT32_MAX, '\n');
    }
  }
  while (result.empty());
}

bool Simulation::load(string filename)
{
  // Open simulator file
  m_simfile.open(filename);
  if (m_simfile.fail())
  {
    cerr << "Unable to open simulator file." << endl;
    return false;
  }

  try
  {
    // Read simulation parameters
    string progFileName;
    string kernelName;
    PARSING("program file");
    get(progFileName);
    PARSING("kernel");
    get(kernelName);
    PARSING("NDRange");
    get(m_ndrange[0]);
    get(m_ndrange[1]);
    get(m_ndrange[2]);
    PARSING("work-group size");
    get(m_wgsize[0]);
    get(m_wgsize[1]);
    get(m_wgsize[2]);

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

      PARSING_ARG(idx);
      get(type);
      m_simfile >> dec;
      get(size);

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
          m_simfile >> hex;
          get(byte);
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
          m_simfile >> hex;
          get(byte);
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
    string next;
    m_simfile >> next;
    if (m_simfile.good() || !m_simfile.eof())
    {
      cerr << "Unexpected token '" << next << "' (expected EOF)" << endl;
      return false;
    }
  }
  catch (ifstream::iostate e)
  {
    if (e == ifstream::eofbit)
    {
      cerr << "Unexpected EOF when parsing " << m_parsing << endl;
      return false;
    }
    else if (e == ifstream::failbit)
    {
      cerr << "Data parsing error for " << m_parsing << endl;
      return false;
    }
    else
    {
      throw e;
    }
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
