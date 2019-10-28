// Simulation.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include <fstream>
#include <list>
#include <sstream>
#include <string>

namespace oclgrind
{
  class Context;
  class Kernel;
  class Program;
};

class Simulation
{
  enum ArgDataType
  {
    TYPE_NONE,
    TYPE_CHAR,
    TYPE_UCHAR,
    TYPE_SHORT,
    TYPE_USHORT,
    TYPE_INT,
    TYPE_UINT,
    TYPE_LONG,
    TYPE_ULONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
  };

  public:
    Simulation();
    virtual ~Simulation();

    bool load(const char *filename);
    void run(bool dumpGlobalMemory=false);

  private:
    oclgrind::Context *m_context;
    oclgrind::Kernel *m_kernel;
    oclgrind::Program *m_program;

    oclgrind::Size3 m_ndrange;
    oclgrind::Size3 m_wgsize;

    std::ifstream m_simfile;
    std::string m_parsing;
    size_t m_lineNumber;
    std::istringstream m_lineBuffer;

    struct DumpArg
    {
      size_t address;
      size_t size;
      ArgDataType type;
      std::string name;
      bool hex;
    };
    std::list<DumpArg> m_dumpArguments;

    template<typename T>
    void dumpArgument(DumpArg& arg);
    template<typename T>
    void get(T& result);
    void parseArgument(size_t index);
    template<typename T>
    void parseArgumentData(unsigned char *result, size_t size);
    template<typename T>
    void parseFill(unsigned char *result, size_t size,
                   std::istringstream& fill);
    template<typename T>
    void parseRange(unsigned char *result, size_t size,
                    std::istringstream& range);
};
