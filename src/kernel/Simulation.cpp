// Simulation.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

#include "core/Context.h"
#include "core/Kernel.h"
#include "core/KernelInvocation.h"
#include "core/Memory.h"
#include "core/Program.h"
#include "kernel/Simulation.h"

using namespace oclgrind;
using namespace std;

#define PARSING(parsing) m_parsing = parsing;

// Convert an integer to char/uchar, checking if the value is valid
#define INT_TO_CHAR(intval, result) \
  result = intval;                  \
  if (result != intval)             \
  {                                 \
    throw "Invalid char value";     \
  }

// Utility to read a typed value from a stream
template<typename T> T readValue(istream& stream);

Simulation::Simulation()
{
  m_context = new Context();
  m_kernel = NULL;
  m_program = NULL;
}

Simulation::~Simulation()
{
  delete m_kernel;
  delete m_program;
  delete m_context;
}

template<typename T>
void Simulation::dumpArgument(DumpArg& arg)
{
  size_t num = arg.size / sizeof(T);
  T *data = new T[num];
  m_context->getGlobalMemory()->load((uint8_t*)data, arg.address, arg.size);

  for (size_t i = 0; i < num; i++)
  {
    cout << "  " << arg.name << "[" << i << "] = ";
    if (arg.hex)
      cout << "0x" << setfill('0') << setw(sizeof(T)*2) << hex;
    if (sizeof(T) == 1)
      cout << (int)data[i];
    else
      cout << data[i];
    cout << dec;
    cout << endl;
  }
  cout << endl;

  delete[] data;
}

template<typename T>
void Simulation::get(T& result)
{
  do
  {
    // Check if line buffer has content
    streampos pos = m_lineBuffer.tellg();
    string token;
    m_lineBuffer >> token;
    if (!m_lineBuffer.fail())
    {
      // Line has content, rewind line buffer
      m_lineBuffer.clear();
      m_lineBuffer.seekg(pos);

      // Read value from line buffer
      m_lineBuffer >> result;
      if (m_lineBuffer.fail())
      {
        throw ifstream::failbit;
      }

      return;
    }

    // Get next line
    string line;
    getline(m_simfile, line);
    m_lineNumber++;

    // Remove comments
    size_t comment = line.find_first_of('#');
    if (comment != string::npos)
    {
      line = line.substr(0, comment);
    }

    // Update line buffer
    m_lineBuffer.clear();
    m_lineBuffer.str(line);
  }
  while (m_simfile.good());

  // Couldn't read data from file, throw exception
  throw m_simfile.eof() ? ifstream::eofbit : ifstream::failbit;
}

bool Simulation::load(const char *filename)
{
  // Open simulator file
  m_lineNumber = 0;
  m_lineBuffer.setstate(ios_base::eofbit);
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
    get(m_ndrange.x);
    get(m_ndrange.y);
    get(m_ndrange.z);
    PARSING("work-group size");
    get(m_wgsize.x);
    get(m_wgsize.y);
    get(m_wgsize.z);

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
      m_program = Program::createFromBitcodeFile(m_context, progFileName);
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
      m_program = new Program(m_context, data);
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

    // Ensure work-group size exactly divides NDRange if necessary
    if (m_kernel->requiresUniformWorkGroups() &&
        (m_ndrange.x % m_wgsize.x ||
         m_ndrange.y % m_wgsize.y ||
         m_ndrange.z % m_wgsize.z))
    {
      cerr << "Work group size must divide NDRange exactly." << endl;
      return false;
    }

    // Parse kernel arguments
    m_dumpArguments.clear();
    for (unsigned index = 0; index < m_kernel->getNumArguments(); index++)
    {
      parseArgument(index);
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
  catch (const char *err)
  {
    cerr << "Line " << m_lineNumber << ": " << err
         << " (" << m_parsing << ")" << endl;
    return false;
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
      cerr << "Line " << m_lineNumber
           << ": Failed to parse " << m_parsing << endl;
      return false;
    }
    else
    {
      throw e;
    }
  }

  return true;
}

void Simulation::parseArgument(size_t index)
{
  // Argument parsing parameters
  size_t size = -1;
  cl_mem_flags flags = 0;
  ArgDataType type = TYPE_NONE;
  size_t typeSize = 0;
  bool null = false;
  bool dump = false;
  bool hex = false;
  bool noinit = false;
  string fill = "";
  string range = "";
  string name = m_kernel->getArgumentName(index).str();

  // Set meaningful parsing status for error messages
  ostringstream stringstream;
  stringstream << "argument " << index << ": " << name;
  string formatted = stringstream.str();
  PARSING(formatted.c_str());

  // Get argument info
  size_t argSize = m_kernel->getArgumentSize(index);
  unsigned int addrSpace = m_kernel->getArgumentAddressQualifier(index);
  const llvm::StringRef argType = m_kernel->getArgumentTypeName(index);

  // Ensure we have an argument header
  char c;
  get(c);
  if (c != '<')
  {
    throw "Expected argument header <...>";
  }

  // Get header
  streampos startpos = m_lineBuffer.tellg();
  string headerStr;
  getline(m_lineBuffer, headerStr);
  size_t end = headerStr.find_last_of('>');
  if (end == string::npos)
  {
    throw "Missing '>' at end of argument header";
  }
  headerStr = headerStr.substr(0, end);

  // Move line buffer to end of header
  m_lineBuffer.clear();
  m_lineBuffer.seekg((int)startpos + headerStr.size() + 1);

  // Save format flags
  ios_base::fmtflags previousFormat = m_lineBuffer.flags();

  // Parse header
  istringstream header(headerStr);
  while (!header.eof())
  {
    // Get next header token
    string token;
    header >> token;
    if (header.fail())
    {
      break;
    }

#define MATCH_TYPE(str, value, sz)                  \
  else if (token == str)                            \
  {                                                 \
    if (type != TYPE_NONE)                          \
    {                                               \
      throw "Argument type defined multiple times"; \
    }                                               \
    type = value;                                   \
    typeSize = sz;                                  \
  }

    // Parse token
    if (false);
    MATCH_TYPE("char", TYPE_CHAR, 1)
    MATCH_TYPE("uchar", TYPE_UCHAR, 1)
    MATCH_TYPE("short", TYPE_SHORT, 2)
    MATCH_TYPE("ushort", TYPE_USHORT, 2)
    MATCH_TYPE("int", TYPE_INT, 4)
    MATCH_TYPE("uint", TYPE_UINT, 4)
    MATCH_TYPE("long", TYPE_LONG, 8)
    MATCH_TYPE("ulong", TYPE_ULONG, 8)
    MATCH_TYPE("float", TYPE_FLOAT, 4)
    MATCH_TYPE("double", TYPE_DOUBLE, 8)
    else if (token.compare(0, 4, "dump") == 0)
    {
      dump = true;
    }
    else if (token.compare(0, 4, "fill") == 0)
    {
      if (token.size() < 6 || token[4] != '=')
      {
        throw "Expected =VALUE after 'fill";
      }
      fill = token.substr(5);
    }
    else if (token == "hex")
    {
      hex = true;
      m_lineBuffer.setf(ios_base::hex);
      m_lineBuffer.unsetf(ios_base::dec | ios_base::oct);
    }
    else if (token == "noinit")
    {
      if (addrSpace != CL_KERNEL_ARG_ADDRESS_GLOBAL &&
          addrSpace != CL_KERNEL_ARG_ADDRESS_CONSTANT)
      {
        throw "'noinit' only valid for buffer arguments";
      }
      noinit = true;
    }
    else if (token == "null")
    {
      if (addrSpace != CL_KERNEL_ARG_ADDRESS_GLOBAL &&
          addrSpace != CL_KERNEL_ARG_ADDRESS_CONSTANT)
      {
        throw "'null' only valid for buffer arguments";
      }
      null = true;
    }
    else if (token.compare(0, 5, "range") == 0)
    {
      if (token.size() < 7 || token[5] != '=')
      {
        throw "Expected =START:INC:END after 'range";
      }
      range = token.substr(6);
    }
    else if (token == "ro")
    {
      if (flags & CL_MEM_WRITE_ONLY)
      {
        throw "'ro' and 'wo' are mutually exclusive";
      }
      if (addrSpace != CL_KERNEL_ARG_ADDRESS_GLOBAL)
      {
        throw "'ro' only valid for global memory buffers";
      }
      flags |= CL_MEM_READ_ONLY;
    }
    else if (token.compare(0, 4, "size") == 0)
    {
      istringstream value(token.substr(4));
      char equals = 0;
      value >> equals;
      if (equals != '=')
      {
        throw "Expected = after 'size'";
      }

      value >> dec >> size;
      if (value.fail() || !value.eof())
      {
        throw "Invalid value for 'size'";
      }
    }
    else if (token == "wo")
    {
      if (flags & CL_MEM_READ_ONLY)
      {
        throw "'ro' and 'wo' are mutually exclusive";
      }
      if (addrSpace != CL_KERNEL_ARG_ADDRESS_GLOBAL)
      {
        throw "'wo' only valid for global memory buffers";
      }
      flags |= CL_MEM_WRITE_ONLY;
    }
    else
    {
      string err = "Unrecognised header token '";
      err += token;
      err += "'";
      throw err.c_str();
    }
  }

  // Ensure size given
  if (null)
  {
    if (size != -1 || !fill.empty() || !range.empty() || noinit || dump)
    {
      throw "'null' not valid with other argument descriptors";
    }
    size = 0;
  }
  else if (size == -1)
  {
    throw "size required";
  }

  if (type == TYPE_NONE)
  {
#define MATCH_TYPE_PREFIX(str, value, sz)       \
  else if (argType.startswith(str))             \
  {                                             \
    type = value;                               \
    typeSize = sz;                              \
  }

    // Set default type using kernel introspection
    if (false);
    MATCH_TYPE_PREFIX("char", TYPE_CHAR, 1)
    MATCH_TYPE_PREFIX("uchar", TYPE_UCHAR, 1)
    MATCH_TYPE_PREFIX("short", TYPE_SHORT, 2)
    MATCH_TYPE_PREFIX("ushort", TYPE_USHORT, 2)
    MATCH_TYPE_PREFIX("int", TYPE_INT, 4)
    MATCH_TYPE_PREFIX("uint", TYPE_UINT, 4)
    MATCH_TYPE_PREFIX("long", TYPE_LONG, 8)
    MATCH_TYPE_PREFIX("ulong", TYPE_ULONG, 8)
    MATCH_TYPE_PREFIX("float", TYPE_FLOAT, 4)
    MATCH_TYPE_PREFIX("double", TYPE_DOUBLE, 8)
    MATCH_TYPE_PREFIX("void*", TYPE_UCHAR, 1)
    else
    {
      throw "Invalid default kernel argument type";
    }
  }

  // Ensure argument data size is a multiple of format type size
  if (size % typeSize)
  {
    throw "Initialiser type must exactly divide argument size";
  }

  // Ensure 'dump' only used with non-null buffers
  if (dump)
  {
    if (addrSpace != CL_KERNEL_ARG_ADDRESS_GLOBAL &&
        addrSpace != CL_KERNEL_ARG_ADDRESS_CONSTANT)
    {
      throw "'dump' only valid for memory objects";
    }
  }

  // Ensure only one initializer given
  unsigned numInitializers = 0;
  if (noinit) numInitializers++;
  if (!fill.empty()) numInitializers++;
  if (!range.empty()) numInitializers++;
  if (numInitializers > 1)
  {
    throw "Multiple initializers present";
  }

  // Generate argument data
  TypedValue value;
  value.size = argSize;
  value.num = 1;
  if (addrSpace == CL_KERNEL_ARG_ADDRESS_LOCAL)
  {
    value.size = size;
    value.data = NULL;
  }
  else if (null)
  {
    value.data = new unsigned char[value.size];
    memset(value.data, 0, value.size);
  }
  else
  {
    // Parse argument data
    unsigned char *data = new unsigned char[size];
    if (noinit){}
    else if (!fill.empty())
    {
      istringstream fillStream(fill);
      fillStream.copyfmt(m_lineBuffer);

  #define FILL_TYPE(type, T)                \
    case type:                              \
      parseFill<T>(data, size, fillStream); \
      break;

      switch (type)
      {
        FILL_TYPE(TYPE_CHAR, int8_t);
        FILL_TYPE(TYPE_UCHAR, uint8_t);
        FILL_TYPE(TYPE_SHORT, int16_t);
        FILL_TYPE(TYPE_USHORT, uint16_t);
        FILL_TYPE(TYPE_INT, int32_t);
        FILL_TYPE(TYPE_UINT, uint32_t);
        FILL_TYPE(TYPE_LONG, int64_t);
        FILL_TYPE(TYPE_ULONG, uint64_t);
        FILL_TYPE(TYPE_FLOAT, float);
        FILL_TYPE(TYPE_DOUBLE, double);
        default:
          throw "Invalid argument data type";
      }
    }
    else if (!range.empty())
    {
      istringstream rangeStream(range);
      rangeStream.copyfmt(m_lineBuffer);

  #define RANGE_TYPE(type, T)                 \
    case type:                                \
      parseRange<T>(data, size, rangeStream); \
      break;

      switch (type)
      {
        RANGE_TYPE(TYPE_CHAR, int8_t);
        RANGE_TYPE(TYPE_UCHAR, uint8_t);
        RANGE_TYPE(TYPE_SHORT, int16_t);
        RANGE_TYPE(TYPE_USHORT, uint16_t);
        RANGE_TYPE(TYPE_INT, int32_t);
        RANGE_TYPE(TYPE_UINT, uint32_t);
        RANGE_TYPE(TYPE_LONG, int64_t);
        RANGE_TYPE(TYPE_ULONG, uint64_t);
        RANGE_TYPE(TYPE_FLOAT, float);
        RANGE_TYPE(TYPE_DOUBLE, double);
        default:
          throw "Invalid argument data type";
      }
    }
    else if (addrSpace != CL_KERNEL_ARG_ADDRESS_LOCAL)
    {
  #define PARSE_TYPE(type, T)           \
    case type:                          \
      parseArgumentData<T>(data, size); \
      break;

      switch (type)
      {
        PARSE_TYPE(TYPE_CHAR, int8_t);
        PARSE_TYPE(TYPE_UCHAR, uint8_t);
        PARSE_TYPE(TYPE_SHORT, int16_t);
        PARSE_TYPE(TYPE_USHORT, uint16_t);
        PARSE_TYPE(TYPE_INT, int32_t);
        PARSE_TYPE(TYPE_UINT, uint32_t);
        PARSE_TYPE(TYPE_LONG, int64_t);
        PARSE_TYPE(TYPE_ULONG, uint64_t);
        PARSE_TYPE(TYPE_FLOAT, float);
        PARSE_TYPE(TYPE_DOUBLE, double);
        default:
          throw "Invalid argument data type";
      }
    }

    if (addrSpace == CL_KERNEL_ARG_ADDRESS_PRIVATE)
    {
      value.data = data;
    }
    else
    {
      // Allocate buffer and store content
      Memory *globalMemory = m_context->getGlobalMemory();
      size_t address = globalMemory->allocateBuffer(size, flags);
      if (!address)
        throw "Failed to allocate global memory";
      if (!noinit)
        globalMemory->store((unsigned char*)&data[0], address, size);
      value.data = new unsigned char[value.size];
      value.setPointer(address);
      delete[] data;

      if (dump)
      {
        DumpArg dump =
        {
          address,
          size,
          type,
          name,
          hex
        };
        m_dumpArguments.push_back(dump);
      }
    }
  }

  // Set argument value
  m_kernel->setArgument(index, value);
  if (value.data)
  {
    delete[] value.data;
  }

  // Reset parsing format
  m_lineBuffer.flags(previousFormat);
}

template<typename T>
void Simulation::parseArgumentData(unsigned char *result, size_t size)
{
  vector<T> data;
  for (int i = 0; i < size / sizeof(T); i++)
  {
    T value;
    if (sizeof(T) == 1)
    {
      int intval;
      get(intval);
      INT_TO_CHAR(intval, value);
    }
    else
    {
      get(value);
    }
    data.push_back(value);
  }
  memcpy(result, &data[0], size);
}

template<typename T>
void Simulation::parseFill(unsigned char *result, size_t size,
                           istringstream& fill)
{
  T value = readValue<T>(fill);
  for (int i = 0; i < size/sizeof(T); i++)
  {
    ((T*)result)[i] = value;
  }

  if (fill.fail() || !fill.eof())
  {
    throw "Invalid fill value";
  }
}

template<typename T>
void Simulation::parseRange(unsigned char *result, size_t size,
                            istringstream& range)
{
  // Parse range format
  T values[3];
  for (int i = 0; i < 3; i++)
  {
    values[i] = readValue<T>(range);
    if (i < 2)
    {
      char colon = 0;
      range >> colon;
      if (range.fail() || colon != ':')
      {
        throw "Invalid range format";
      }
    }
  }
  if (range.fail() || !range.eof())
  {
    throw "Invalid range format";
  }

  // Ensure range is value
  double num = (values[2] - values[0] + values[1]) / (double)values[1];
  if (ceil(num) != num || num*sizeof(T) != size)
  {
    throw "Range doesn't produce correct buffer size";
  }

  // Produce range values
  T value = values[0];
  for (size_t i = 0; i < num; i++)
  {
    ((T*)result)[i] = value;
    value += values[1];
  }
}

void Simulation::run(bool dumpGlobalMemory)
{
  assert(m_kernel && m_program);
  assert(m_kernel->allArgumentsSet());

  Size3 offset(0, 0, 0);
  KernelInvocation::run(m_context, m_kernel, 3, offset, m_ndrange, m_wgsize);

  // Dump individual arguments
  cout << dec;
  list<DumpArg>::iterator itr;
  for (itr = m_dumpArguments.begin(); itr != m_dumpArguments.end(); itr++)
  {
    cout << endl
         << "Argument '" << itr->name << "': "
         << itr->size << " bytes" << endl;

#define DUMP_TYPE(type, T) \
  case type:               \
    dumpArgument<T>(*itr); \
    break;

    switch (itr->type)
    {
      DUMP_TYPE(TYPE_CHAR, int8_t);
      DUMP_TYPE(TYPE_UCHAR, uint8_t);
      DUMP_TYPE(TYPE_SHORT, int16_t);
      DUMP_TYPE(TYPE_USHORT, uint16_t);
      DUMP_TYPE(TYPE_INT, int32_t);
      DUMP_TYPE(TYPE_UINT, uint32_t);
      DUMP_TYPE(TYPE_LONG, int64_t);
      DUMP_TYPE(TYPE_ULONG, uint64_t);
      DUMP_TYPE(TYPE_FLOAT, float);
      DUMP_TYPE(TYPE_DOUBLE, double);
      default:
        throw "Invalid argument data type";
    }
  }

  // Dump global memory if required
  if (dumpGlobalMemory)
  {
    cout << endl << "Global Memory:" << endl;
    m_context->getGlobalMemory()->dump();
  }
}

template<typename T>
T readValue(istream& stream)
{
  T value;
  if (sizeof(T) == 1)
  {
    int intval;
    stream >> intval;
    INT_TO_CHAR(intval, value);
  }
  else
  {
    stream >> value;
  }
  return value;
}
