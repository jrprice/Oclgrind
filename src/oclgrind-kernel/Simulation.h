// Simulation.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include <fstream>
#include <sstream>
#include <string>

namespace oclgrind
{
  class Device;
  class Kernel;
  class Program;
};

class Simulation
{
  public:
    Simulation();
    virtual ~Simulation();

    bool load(const char *filename);
    void run(bool dumpGlobalMemory=false);

  private:
    oclgrind::Device *m_device;
    oclgrind::Kernel *m_kernel;
    oclgrind::Program *m_program;

    size_t m_ndrange[3];
    size_t m_wgsize[3];

    std::ifstream m_simfile;
    std::string m_parsing;
    size_t m_lineNumber;
    std::istringstream m_lineBuffer;

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
